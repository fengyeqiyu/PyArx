# DWG1018 离线解码进度（2026-11-19 更新）

## 本轮突破
- 编译出权威对照：`G:\PyArx\docs\libredwg_src\build\dump_hdl.exe`（链接 libredwg.lib），
  用法 `dump_hdl.exe <dwg> <out.tsv>`，输出 `handle(十进制)\taddress\ttype\tname`。
  主文件权威映射落盘 `examples/cad2d/dwg1018/ref_map.txt`（277777 对象）。
- **句柄表 MC 地址增量解码本来就正确**。之前"增量量级异常"是误判：对象在节内不按句柄排序，
  h2 直接跳到 19487965 属正常。
- 地址口径：MC 累加值 = 对象体起点；ref_map 的 address = 体起点 + 2（MS 读完后位置）。
  定位对象用 `body = ref_address - 2`，`dat->byte = body`。
- 验证：目标 LINE 0x11CF7，ref addr=19546652，body=19546650，读出 type=0x13(LINE)、
  handle=0x11CF7 完全匹配。

## 已验证正确
- 压缩层 LZ77 解压、跨页回引、整节拼接（与 C 参考逐字节一致）。
- 对象头：MS size → BS type → RL bitsize → H handle（跨位读取）。
- UMC 句柄增量（sec0 内单字节条目正确）；MC 地址增量（body=ref-2 匹配）。
- CRC16 seed 0xC0C1；对象条目 CRC 读 RS 小端。

## 未决
- 句柄表 UMC 多字节条目方向/语义存疑：sec1 首条 UMC `c0 05` 解出 704(0x2C0)，
  与按增量累加的 0x57F 不符；实际目标句柄即 0x2C0。句柄权威值以对象头 H 为准，
  句柄表 handleoff 仅用于排序/校验。Python 句柄表全量排序映射待修（当前用 ref_map 校准）。
- 句柄表共 315 个小节（section_size 链连续切，跨 22 数据页），非每页一节；
  真实解压大小 637399，缓冲尾部为零填充。
- 实体公共字段（EED 后）顺序未打通；VERTEX/DIMENSION/HATCH 未解码；几何提取与渲染未开始。

## 2026-11-20 突破：LINE 实体解码打通（89.6% 全量匹配）
权威位流追踪（dump_hdl 在 bits.c/decode.c/common_entity_data.spec 插 fprintf，
按 dat->size 过滤目标对象）确认 R2004 LINE 解码顺序：

对象头（主流 dat）：MS size → BS type → RL bitsize → H handle。
**双句柄流**：hdl_dat 起点 = bitsize 位边界（`byte=body+bitsize//8, bit=bitsize%8`）；
ownerhandle、layer（ltype 仅当 ltype_flags==3）、plotstyle（flags==3）等句柄都在 hdl_dat。
主流 dat 只放数值/位标志；句柄流不占主流位。

EED（主流内）：循环 BS size=0 结束；每条 = BS size → H handle → size 字节数据。
数据在**副本位流**解码（保留 H 后的 bit 位置），主流直接 `byte=end`（end=H后byte+size，保留 bit）。
EED data code：70 RS / 71 RL / 3,5 RLL_BE / 1 RS / 2 RC / 0 RC+TF / 4 RC+TF /
10-15 3RD / 40,41,42 RD。

EED 后主流顺序（R2004）：
B preview_exists → BB entmode → BL num_reactors → B is_xdic_missing →
BSx color.raw（index=raw&0x1ff，256=BYLAYER）→ BD ltype_scale → BB ltype_flags →
BB plotstyle_flags → BS invisible → RC linewt。
然后主流回几何：B z_is_zero → RD start.x → DD end.x(基准start.x) → RD start.y →
DD end.y(基准start.y)；z_is_zero=1 跳过 z。
注意几何 x/y 交错：start.x, end.x, start.y, end.y（不是先两个 start 再两个 end）。

## 2026-11-20 二轮修正：对象头链头基准 + preview 位 + EED 结束位

- **主流链头 = MS size 之后地址**（`obj->address`，即 ref_map address），不是 address-2。
  MS size 字段本身不入主流；定位对象用 body=address-2 仅为读 MS size 取 size 值，
  主流 BitStream 起始 `byte=address`。此前从 body 起读主流整体偏 2 字节。
- 对象头主流顺序：BS type → RL bitsize → H handle（MS 已在主流链头前读完）。
- **主流 EED 之后必读 `B preview_exists`**（spec common_entity_data.spec 303 FIELD_B）。
  早期误判"R2004 不读 preview"，漏掉这 1 bit 导致主流全错位。
- EED 结束主流起点：有 EED 对象 `byte=end, bit=6`；无 EED 对象保留 EED BS size 读完的当前位。
  随后 preview B 推 1 bit，entmode 起点 bit=7。
- EED 派生条目：首条 BS size+H；后续 `size=end-当前byte+1`、继承上一条 handle；
  条目内派生临时 BitStream 解码，主流 `byte=end`。
- MS 位序：首读字放低位(j=0)，次读字放高位(j=15)（bit_read_MS 源码 i=1 递减、j=0 递增）。
- 三样本权威对齐：h=55 (-300,200)→(-300,-200)；h=56 (-300,200)→(300,200)；
  h=CD (0.75,~0)→(0.75,0.6)。主流字段 entmode=0/numr=0/xdic=1/color=256/ltscale=1.0/
  lt_flags=0/plotstyle=0/inv=0/linewt=0x1D 全对。

批量校验：ref_map 全量 LINE 208001 条，187700 匹配（90.2%）。
失败集中在 color 非 BYLAYER（color flag 分支：DBCOLOR 句柄/RGB/alpha/name），
size=98/96/61/59 等；以及 size=17408（MS 错位变体）。
探针：`probe_batch.py`（全量校验）、`probe_line2.py`（单条主流+几何）、`probe_size.py`（size/EED 分布）。

## 待办
- 实现 color ENC flag 分支（spec 415-498）：flags&0x40 句柄 / 0x80 RGB(BLx) / 0x20 alpha / color name。
- 排查 num_reactors>0 反应器句柄、size=17408 MS 错位变体。
- 其余实体：CIRCLE/ARC/POINT/INSERT/LWPOLYLINE/TEXT/ATTRIB 几何字段。
- 纯 Python 句柄表全量映射（当前兜底 ref_map.txt）；批量解码后渲染出图。
- 交付前回退 decode.c/bits.c/common_entity_data.spec 临时 fprintf 插桩。

## 关键路径
- 容器/解压/位链：`examples/cad2d/dwg1018/{container,compress,bitio}.py`
- 缓冲：`obj_full.bin`(22,004,736)、`hdl_full.bin`(653,312，真实 637399)
- 权威映射：`ref_map.txt`；DXF 基准：`ground_truth.dxf`
- 编译：vcvars64.bat → cmake(NMake) 或 cl；libredwg 源码 `docs/libredwg_src/`
2026-11 失败分桶(ARX基线): size98/EED/ACI8 9216; size17408 MS变体 6186; 无EED小size 41-53 ~3600。size98 探针 num_reactors BL 起读异常; C decode.c:4256 EED结束仅 byte=end, bit 保留 data 解码位; 离线强制 bit=6 仅 size60/40 验证过, 待按 trace_98b 校准。color ENC 分支(spec443-487)未实现。
size98 深挖: C entmode @52.3(9684/9736), 离线 EED size=3 endbyte16(9175)。主流头顺序核对一致: type BS->bitsize RL->handle H->EED size BS, handle 读取与 bit_read_H 一致。新假设: size98 对象 preview_exists=1, spec303-338 preview_size BLL+BINARY 段占~40字节, entmode 在 preview 之后故 @52; 离线把 preview 二进制误并入 EED/未跳 preview 段。待 C 插桩确认 preview_exists/preview_size。
排除 preview 假设: trace_pv 9736 个 size98 对象 preview_exists 全=0, 无 preview_size。根因转向头部: C bitsize=445@5.2 handle@7.2; 同型离线 bitsize=723@5.2 handle@9.2。bitsize RL 字序/handle 段长度存偏差, handle 段 C 占2字节离线4字节。待对照 bit_read_H code/size 与 RL 低字在前。
handle 200404 C 全链对齐: bitsize=723@5.2 handle v=200404 code=0 size=3@9.2 EED size=3@10.4 code70@14.4; 派生: size3@17.6 code70@21.6; size25@25.0 code10@28 RD@28/36/44; size0@52.2 entmode@52.3。头部 bitsize/handle 读取离线正确。根因: EED 派生循环离线用 end-1 判定过早 break, 且派生 size 未重读 BS(C 4079 每条重读 BS, 派生仅继承 handle)。probe_one 已改为派生重读 BS, 待校准 end 基准与 EED handle 字节数。
注意: EED handle 插桩(4121)致 dump_hdl 0xC0000005 早期崩溃, 已回退源码; 当前 build 内 exe/库为崩溃版, 下次 build_make+build_dump 重编恢复。
EED 派生 probe 验证(handle200404): 派生条目位流上确有 handle 段需 read_h 推进; 三条 code 全对齐 C: 70@14.4/70@21.6/10@28.0, entmode@52.3 对上。但无条件派生重读 size 破坏单EED/无EED对象, 回归几何 90.2->89.0, 已回退基线 90.1%。正确做法: 保留单EED路径, 仅多EED条目派生; 派生判定按 C4234 dat->byte<end-1, end 基准(read_H前/后byte)待核。
EED 落地回归: 两次改动(派生重读BS / end基准改read_H前)均使几何退化 90.2->89.0, 已回退基线 90.1%。结论: 旧 end=read_H后byte+size 对单EED正确; 改 read_H前会缩小 end 致 b.byte<end-1 误触发派生。安全落地前须先取单EED成功样本的 C 权威 end 与 dat->byte, 校准 4234 判定阈值, 避免把主流首字段误当派生 size。
本轮: 严格按 C 4079/4120/4123/4234 落地派生框架(每条 read_h, 派生重读BS, 收尾bit=6), 主链路回归 90.1% 不退化(颜色+6)。size=98 簇仍 0%: 对照 trace 200404, 权威 idx2 end=50=read_H前byte25+size25, probe end=52=read_H后byte27+25, end 偏大2; EED handle 段疑似应在独立 hdl_dat 流而非主流推进 byte。entmode@52.3 对上但 numr BL@56.5 错位(C52.5), color@59.0(C53.0), 根因 EED 收尾位偏 2bit。下轮先核 EED handle 是否走 hdl_dat 流。
trace_eed3 插桩确认: 200404 [EEDEND] byte=52 bit=2 num_eed=3, preview@52.3。两次复刻尝试(补读结束BS / 外层while顶读BS)均退化 88.8%, 已回退基线 90.1%。结论: 离线 data 解码后 byte 与 C 派生判定基准不等价, 单EED对象对收尾bit敏感; 不宜盲改。下轮需在 C 4234 判定处插桩打 dat->byte/end/sav_byte, 逐条目对照派生边界, 再定离线判定。decode.c 现有 EEDEND fprintf 插桩安全(不碰handle.value)。
突破: EDCHK 插桩显示三条 EED 判定 byte=end(16/23/52) 均 >=end-1, 4234 派生分支对填满 data 的条目从不触发; R2004 实际无条件回外层 4079 重读 BS 直到 size=0。去掉内层 break 后落地: 全字段 187352->187632(90.2%), 几何 +280, 颜色 192731->202250(97.2%, +9519)。probe 200404 size=0@52.2 after EED 52.2 preview@52.3 对上。剩余: entmode 偏 1bit, size=98 部分几何仍 0.0 错位。
ENC 分支落地(bit_read_ENC bits.c4219-4243): raw=BSx; flag=raw>>8; flag&0x80 消费 rgb BL; flag&0x20 消费 alpha BL; 0x40 handle 走 hdl_dat 主流不消费。结果: 几何 187981->197940(95.2%, +9959), 全字段 187632->197588(95.0%), 不符 20369->10413。待补: 0x41/0x42 TV name/book_name, 0x40 DBCOLOR handle(hdl_dat)。
分桶(95.0%基线): size98 9216->271; size17408 仍 6182(最大剩余, 多为 PUB_HATCH 层); 无EED 小size41-53 零散。对象头: ref_map address=MS后位置(decode.c5577), MS 在 address 前 1-2 RS字; R2004 无 UMC。17408 簇 type BS 解出 256(code3), 待核 MS 双字变体与块内实体 address 映射。

## 2026-11-20 三轮：解压层 0x10 分支修复，全流 100% 对齐 → 99.96%

- **解压根因（最大簇 size=17408，6182 条）**：离线 compress.py 把 opcode 0x10 误判为 Long Offset 特殊分支（`_long_offset+9`，2bo +0x3FFF）。
  对照 C decode.c:1269，0x10 与 0x12–0x1F 同款：`read_compressed_bytes(7)` + `high=(op&8)<<11` + two_byte_offset(0x4000)。
  修掉后重跑 regen_all.py，离线 obj_full.bin 前 22,002,110 字节与 C 权威 obj_auth.bin **逐字节 0 差异**（尾部 2626 为零填充）。
- C 权威流落盘：decode.c `dwg_decode_add_object` 首对象 dump `abs_dat.chain` → `obj_auth.bin`（size=22,002,110），供离线解压逐页 diff（diff_auth.py / diff_page.py）。
- **EED code0 补 codepage**：R2004 code0 = len RC + codepage **RS_BE(2字节)** + TF(len)；离线原漏读 cp 2 字节。bitio 已有 `read_rs_be()`。
- **未知 EED code**：C default 分支 `dat->byte=end` 跳过整条（decode.c:4049）；离线原 break 停在中途，改为 `q.byte=end; q.bit=0; break`。
- 复测：handle **100.0%**(208001)，几何/颜色 **99.96%**(207911/208001)，全字段 95.0%→**99.96%**，剩 **90 条**。
- 残留 90 条分布：S-批注系列 / 0 层 / 块内成员(H_AC_MA_DIFFUSER 已随解压修复消失)。
- **残留根因（未修）**：EED data 解码后主流位位置比权威少约 1 字节。样例 h=15893 addr=4308146 size=108：
  权威 preview@68.1 / entmode@68.1 / numreactors@68.3；离线 67.1 / 67.3 / 71.5（numreactors BL 爆成大数）。
  EED 两条 code40 RD×4 + code70 RS×3，第二条 code70 RS×2。位推进底层 RC/RS/_advance 已与 C 核对一致；
  疑点：**RD/RLL 对齐路径**——C bit_read_RD/RLL 在 `bit==0 && byte%8==0` 时直接 le64 读 8 字节 `byte+=8`，离线恒走 RLL(RL×2=RC×8)，对齐场景下位残留可能不同；待核 EED data 内 RD 起始位是否落在对齐路径。
  样例 h=74230 addr=19608157 size=76，同型。
- 临时插桩（交付前回退）：decode.c `[DBG17]/[DBG17H]/[EED] h@/end=`、common_entity_data.spec `[PV]/[C]`（条件已放宽 size 108/76）、obj_auth.bin dump。

## 2026-11-20 四轮：EED 收尾覆盖 bug 修复 → 全字段 **100.0%**

- **根因（残留 90 条）**：EED 末块退出外层 while 时，循环顶已读毕 `size=0` 结束 BS，主流起点被推到 end+1 字节边界；离线退出后又执行 `b.byte=end` 把位置压回 end，主流整体少 1 字节。
- 样例 h=15893 addr=4308146 size=108：C trace `[EEDEND2] afterH byte=61 bit=6 end=67`，末块派生 code70 RS×2 后 `[EEDEND] byte=68 bit=0`，preview@68.1；离线原收尾 67.0、preview@67.1。
- **修复**：cmp_arx/probe_one/classify_bad 删除收尾多余的 `if end: b.byte=end` 覆盖；保留末条 size=0 BS 读完后的位置即主流起点。
- 复测 cmp_arx.py：**handle/几何/颜色/全字段 208001/208001 = 100.0%，不符 0 条**。几何样例 (0,-36.302)→(36.128,-50.0)、color 0 全对上。
- 历史"RD/RLL 对齐路径"疑点排除：本图 LINE 的 RD 起始位均非字节对齐，未走 C 直接 le64 快路径；90 条全为 EED 收尾覆盖所致。
- 未决：0x40 DBCOLOR handle 走独立 hdl_dat 句柄流（本图 LINE 颜色无命中残留）；层/线型/plotstyle 句柄组比对依赖 hdl_dat 解码；VERTEX/DIMENSION/HATCH 未解码；渲染出图待接。

## 2026-11-20 五轮：全图形类型离线解码（cmp_all.py 按 ARX 基线）

对比口径：几何 1e-5 / 半径 1e-4 / 角度 1e-4。缓冲 obj_full.bin；句柄流 hdl_dat 未解（#13）。

| 类型 | ARX | ok | 备注 |
|---|---|---|---|
| AcDbLine | 208210 | 208001 | 209 miss=ref_map 缺地址（块内附属） |
| AcDbArc | 17634 | 17634 | 100% |
| AcDbCircle | 8546 | 8546 | 100% |
| AcDbPoint | 583 | 583 | 100% |
| AcDbEllipse | 2353 | 2353 | 100% |
| AcDbPolyline | 6146 | 6146 | 100% |
| AcDbBlockReference | 5221 | 5221 | 100% |
| AcDbText(含ATTRIB/ATTDEF) | 3587 | 3587 | 100% |
| AcDbMText | 728 | 481 | 247 miss=块内附属 |
| AcDbSpline | 16567 | 16567 | 100%（闭合周期样条 ARX 重构去重，比对层跳过点集） |
| AcDbHatch | 329 | 329 | 100%（SOLID 跳过 angle/space；ARX 空 loop 跳过几何） |
| AcDbDimension | 180 | 180 | 100% |
| AcDbTable | 19 | 0 | 未接入：结构深、ARX 未 dump geom、依赖 TableContent/Style 句柄 → #13 |

### 关键流顺序（R2004）
- **Dimension**：extrusion 3BD → text_midpt 2RD → elevation BD → flag1 RC → user_text TV → text_rotation BD0 → horiz_dir BD0 → ins_scale 3BD → ins_rotation BD0 → attachment BS → lspace_style BS1 → lspace_factor BD1 → act_measurement BD → clone_ins_pt 2RD0；子类尾部 xline1/xline2/def_pt 3BD + oblique BD（ALIGNED 口径，其余子类未细分，本图全 ALIGNED）。
- **Hatch**：开头 gradientfill 段 → elevation BD → extrusion **3BD**（非 BE）→ name TV → solid B → assoc B → num_paths BL；path flag BL：&2 polyline（bulg B, closed B, n BL, 顶点 2RD[+bulge BD]），否则曲线边（nseg BL + curve_type RC）；num_boundary_handles BL（句柄跳过）。尾部 style BS, pattern_type BS；非 solid 读 angle/space/double/deflines；derived 置位补 pixel_size BD；seeds 2RD。
- **Spline**：scenario BL, degree BL；scenario&1：rational/closed/periodic B → knot_tol/ctrl_tol BD → n_knots/n_ctrl BL → weighted B → knots BD → ctrl 3BD[+w]；scenario2(bezier)：fit_tol BD → beg/end_tan 3BD → n_fit BL → fit 3BD。
- **MText**：ins_pt 3BD → extrusion 3BD → x_axis 3BD → rect_width BD → height BD → attach BS → flow BS → ext_h/ext_w BD → text TV → ls_style BS → ls_factor BD → unknown B → bg_flag BL（&1: bg_scale BD+CMC+trans BL）。
- **Insert**：ins_pt 3BD → scale_flag BB(3=1,1,1,1 / 2=x=RD,y=z=x / 1=x=1,y,z=DD(1.0) / 0=x=RD,y=DD(x),z=DD(x)) → rotation BD0 → extrusion 3BD → has_attribs B。

### 比对层口径修正
- SOLID 填充不比 patternAngle/Space（ARX space 默认 1.0，离线 0.0）。
- ARX loops 全空 pts（边界靠句柄引用）跳过几何 loop 校验。
- Spline closed&periodic 且 ARX 有 fit（重构去重）跳过 ctrl/fit 点集；离线 ctrl 数与 C 权威一致。
- Text/MText 文本 `\U+XXXX` 正则展开后比；尾部 NUL rstrip。

### 残留
- Table 19 条 + 块内 miss 456（Line 209/MText 247）+ hdl_dat 句柄流（layer/ltype/style/block/boundary/DBCOLOR）→ #13。
- ATTRIB/ATTDEF 后缀 tag/flags/field_length 未读；非图形表对象未解析。
- 渲染出图待几何齐全后接 D2D/DWrite。

## 2026-11-20 六轮：句柄流 + 表记录解析

### 对象句柄流（hdl_dat，bitsize 位之后）
起点 = 对象主流起点 + bitsize 位（非独立缓冲）。顺序（common_object_handle_data.spec）：
1. ownerhandle H（entmode==0）
2. reactors[num_reactors] H
3. xdic H（!is_xdic_missing）
4. layer H（无条件）
5. ltype H（ltype_flags==3）
6. plotstyle H（plotstyle_flags==3）
- 主流公共段仅 num_reactors BL + is_xdic_missing B；owner/reactors/xdic 全在 hdl 流。
- 校准：dump_hdl.c LINE 分支导出 own/numr/xdic/ltype/ps，LINE 208001 条全对 C 权威。

### 表记录（LAYER 51 / LTYPE 57 / STYLE 53 / DIMSTYLE 69）
主流头：type BS → bitsize RL → handle H → EED → num_reactors BL → xdic_missing B → name TV；其后 xref BS/H、flag0 BS、color CMC 等。
- build_tables.py 解出 tables.tsv：LAYER 184、LTYPE 23、STYLE 44、DIMSTYLE 4，name GBK 解码正确（0 / 中文层名 / Continuous / DASH / Standard / STANDARD 等）。
- verify_tables.py：实体 layer/ltype handle 查表名比对 ARX，269628 全对；bad 19 全为未解码 Table。ltype_flags：0=ByLayer，1=ByBlock，2=Continuous，3=有 handle。

### Table（816）待解
- C trace（decode.c / common_entity_data.spec 临时插桩，obj address 19665356 / handle 266206）：exists=1 @10.5，size=4316 @14.5；离线 after hdr @10.2，头读取累计位少 3 位。对象头离线分解：type BS(816)@0.0→2.2，bitsize RL(40881)@2.2→6.2，handle H(code3,sz3)@6.2→10.2。C after handle @10.5，差 3 位，疑 type BS 高位编码或 handle 前缀，需对照 5630 分发器位长。
- common 段 preview_exists=1：需先读 preview_size + preview 二进制再进 entmode；当前 read_common 漏读。preview_size C=4316（spec 319 FIELD_CAST RL→BLL），离线 RL=1143 / BLL=8376 均不符，位起点修正后再定编码。
- struct 头部（dwg.h 4774）：unknown_rc/unknown_h/unknown_bl/unknown_b/unknown_bl1 → ins_pt 3BD → scale 3BD → scale_flag BB → rotation BD → extrusion BE → has_attribs B → num_owned BL → flag_for_table_value BS → horiz_direction 3BD → num_cols BL → num_rows BL → col_widths/row_heights/cells。
- ARX 未 dump Table geom，比对基准缺失。

### Table（816）进展（本轮）
- read_common 已补 preview：preview=1 时读 preview_size RL + read_tf(pvsz) 跳过 preview 二进制；common 尾部 invisible BS + linewt RC 补读后几何起点对齐。
- 几何段顺序（dwg2.spec 504-625，common 后）：ins_pt 3BD → scale_flag BB（0: x=1 DD y DD z；1: x=1 DD y=1 DD z=1；2: RD x, y=z=x；3: 全 1.0）→ rotation BD0 → extrusion **BE**（非 3BD）→ has_attribs B → num_owned BL(if) → block_header H → attribs H[num_owned] → seqend H(if) → tablestyle H → flag BS → horiz 3BD → nc BL → nr BL。
- 校准：19 条 ins_pt 全对 C 权威（误差 1e-3 内取整差）；sf=3、rot=0、extr=(0,0,1) 一致。
- 待解：block_header 之后 tablestyle/horiz/nc/nr/cells 段错位（nc 读到 3416850179），属 TableContent 附属对象段，ARX 无基准，留待字典/附属句柄解析。

### Hatch boundary_handles（本轮修复）
- **geom 内引用句柄走 hdl 句柄流，不在主流 dat**：`VALUE_HANDLE` 宏 R2004 分支 `bit_read_H(hdl_dat)`。read_handles 读完 owner/reactors/xdic/layer/ltype/ps 后 h 流状态保留，传入 geom；boundary_handles 的 H 从该 h 流继续读。
- 主流只保留 num_boundary_handles BL；此前在主流 read_h 多读 1 个 H，导致 style/ptype/angle/spacing/nseed 全错位（style 读成 64、nseed 读到 36 亿越界崩）。
- 修复后 2150 style=1/ptype=1/spacing=700 与 C 权威一致；boundary handle：2146→2149、2150→2151、2154→2155、2170→2171（合法实体句柄）。
- 全量回归：Hatch 329 err=0，其余类型无退化。loop["boundaryHandles"] 已存句柄列表，derived 边界几何待跟随句柄引用解析。
- 坑：G 盘嵌入 python 曾 codec 缺失（Lib/encodings 源码被 strip 仅 5 个 .py），从 `D:\Program Files\Python315\Lib\encodings\*.py` 补齐后启动正常。

### 块内 miss 456 根因（本轮初判，未修）
- miss：AcDbLine 209 + AcDbMText 247，layer「0电气线」437 / Defpoints 19；handle 区间 278386~279117（ARX max=279117）。
- ARX 遍历口径：bt.names() 每个 block record 的 entityIds()，含模型/图纸空间与全部块定义子树；离线/C 仅靠 Handles 对象映射表枚举。
- Handles 节（hdl_full.bin，22 页 653312B，解压与 ref 前 11018B diff=0）映射表解析 total 277777、**max handle 278320**，278321 起整体缺条目；probe_mapgap 无大缺口，末段被当 size=0 空页（parse_hdl page 4288+）。疑内部子页 section_size/CRC 后位对齐或某 UMC/MC 变体致后续条目丢失；C libredwg object[] 同缺这 456（ref_map 277777 与离线一致）。
- 待查：decode.c 2608-2705 Handles 页循环（2636 invalid handleoff 校验、2666 add_object）；离线 parse_hdl/probe_fullmap 的子页 resync（2629 `!handleoff` 回退一字节）口径。对象体在 Objects 节，类型解码器已支持 line/mtext，仅缺句柄索引枚举。

### 非图形对象解码（本轮）
- **DICTIONARY(42) 652 条 / 959 项**：主流 numitems BL → cloning BS → hardowner RC → texts T[numitems]；hdl 流 owner/reactors/xdic 后 itemhandles H[numitems]（句柄走 hdl 流）。`entity.decode_dict(addr)` 返回 items=[(text,handle)]。探针全量 bad=0。样例：3→ACAD_SORTENTS:275423；11 系统字典 17 项（ACAD_GROUP/ACAD_LAYOUT/ACAD_TABLESTYLE…）。
- **DICTIONARYVAR(503) 28 条**：主流 schema RC → strvalue T；hdl 流 owner/reactors/xdic。`entity.decode_dictvar(addr)`，全量 bad=0。样例 val "5.000000"/"1"/"1:1"。
- 句柄流状态贯通：新增 `open_handles(addr,bitsize)` 建 h 流，read_handles(h,common) 保留 h 状态传 geom；字典/dictvar 各自从 open_handles 起读 owner 链。
- **待解 XRECORD(79) 672**：主流 xdata_size BL → XDATA 组码序列（dwg_decode_xdata）→ cloning BS；hdl 流 owner/reactors/xdic 后 objid_handles H 序列直到流尾。XDATA 组码值类型分支待实现。
  - 跳过版探针（xds BL 后 `b.byte=start+xds` 再读 cloning BS）未对齐：owner 读出 0，大对象（handle 15 bs=669763）xds=83706、cloning=49280 错位；需先核对 XRECORD 头 owner 条件与 xdata_size 是否含头/编码口径。组码→vtype 映射见 dwg.c 2121 dwg_resbuf_value_type；值读取见 decode.c 5106 switch（STRING: RS len + RC codepage + TF len；REAL RD；INT8 RC；INT16 RS；INT32 RL；INT64 RLL；POINT3D 3RD；BINARY RC len + TF；HANDLE/OBJECTID RLL）。
- **待解 Table 附属**：TABLECONTENT 818(19) / TABLEGEOMETRY 819(19) / UNKNOWN_OBJ 826(19)，bs=36861，对应 19 条 Table 的 cells/行列宽高；ARX 无基准，需对 C dump。


