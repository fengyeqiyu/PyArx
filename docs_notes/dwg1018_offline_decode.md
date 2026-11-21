# AC1018 (R2004) DWG 离线解析进度（不依赖 CAD 环境）

样本：`中试车间扩建-工艺配电及系统2026.10.12_t3.dwg`，版本 **AC1018**（R2004），11.8 MB。
约束：不启动 AutoCAD / 不引入 LibreDWG、ODA、Teigha 二进制；纯 Python 自解，规范/源码仅作参照。
参照资料：
- `docs/dwg_spec/ODA_DWG_Spec_v5.1.pdf`（§4.1–4.7 R2004 格式与压缩）
- `docs/dwg_spec/decode_ref.c`（LibreDWG `decompress_R2004_section`，仅对照逻辑，未链接其库）
- `docs/dwg_spec/bits_ref.c`（确认 `bit_read_RC` 为纯字节读取，非位流）

## 已打通（容器层）

1. **文件头解密**：0x80 起 0x6C 字节，按 LCG 魔数串（seed=1, `0x343FD*seed+0x269EC3`，取 `>>16` 低字节）逐字节 XOR；
   解出魔数串 `AcFssFcAJMB`，字段见 `examples/cad2d/dwg1018/container.py`。
2. **系统页定位**：签名小端
   - PageMap `0x41630E3B`，SectionMap `0x4163003B`；页头 0x14：type(4) decomp(4) comp(4) ctype(4) checksum(4)。
   - 每个签名在首尾各一份，当前取首份（有效副本 decomp/comp 非 0；尾副本全 0）。
3. **Section Page Map（§4.4）**：条目 `(pageId:int32, sectionSize:uint32)`；pageId<0 为 gap，后随 parent/left/right/0。
4. **Section Map（§4.5）**：NumDescriptions 起，每条 size(8)/pageCount(4)/maxDecomp(4)/unknown(4)/compressed(4)/id(4)/encrypted(4)/name(64)，
   后随 pageCount 个 `(pageNo:4, dataSize:4, startOffset:8)`。
5. 数据页头（§4.6）：32 字节，`secMask = 0x4164536B ^ offset`，8 个 uint32 各自异或；type 恒 `0x4163043B`。

## 卡点（压缩层）

- R2004 18-bit LZ77 变体（§4.7）：PageMap decomp=6192 / comp=5084；解压前 6 条页地图条目正确
  （id 1–6，size 1056/416/672/26816/22912/23104），第 7 条起错位；在 out≈3204、op=0x24 时偏移 3308 越界。
  SectionMap 同类错位（out≈1934，off=2443）。
- 已核对并排除：
  - `bit_read_RC` 为纯字节读取（bits_ref.c 288），非位流；
  - 流尾以 `0x11` 正常结束，无独立 RS 段；
  - 偏移拼接 `(first>>2)|(second<<6)` 与规范 §4.7 Two Byte Offset 一致；
  - 长度公式 0x12–0x1F `(op&0xF)+2`、0x21–0x3F `op-0x1E`、0x40–0xFF `((op&0xF0)>>4)-1` 与规范表一致；
  - 系统页签名各两份，首份有效（decomp/comp 非 0），尾份全 0；
  - 常规匹配 litCount = opcode1&3；Two Byte Offset 分支 litCount = first&3。
- 待排查（下一轮）：
  1. 0x21–0x3F 偏移基值：规范表未写 plus，参考源码加 1；0x12–0x1F 规范 0x3FFF vs 源码 0x4000+(op&8)<<11，需逐字节验证；
  2. litCount==0 时字面长度 opcode 的回退（高半字节非 0 即下一 opcode）是否在常规匹配分支误读；
  3. 0x10 / 0x20 长匹配 Long Offset 分支本样本未命中；
  4. 常规匹配 op2 高位是否并入偏移（18-bit 窗口）。

## 已解析的文件头（本样本）

- version AC1018，maint 104，codepage 39，security 0；
- page_amount 0x306（774），page_map_id 776 @0xBD0980，section_map_id 775，page_array_size 776；
- second header @0xBD1E84，last_end 0xBD44A0。

## 目录

- `examples/cad2d/dwg1018/container.py` 容器层（头/页地图/节地图/数据页定位）
- `examples/cad2d/dwg1018/compress.py` R2004 解压（迭代中）
- `examples/cad2d/dwg1018/analyze.py` 结构报告入口（解压失败仍输出容器信息）
- `examples/cad2d/dwg1018/probe_*.py` 调试探针
