# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
# 页9 start 267264 page 8000 -> 末 275264; 页10 start 296960
for off in [275264, 296960-16, 296960, 296960+8192-16]:
    print(f"@{off}", data[off:off+16].hex(' '))
# 页9 section 结尾 CRC 位置: 页9 解析 end? 用 parse 已知 page9 size=2033 但跨多 section
# 找页10 前 0x3101 模式起始
print("gap 275264..296960 nonzero:", sum(1 for x in data[275264:296960] if x))
