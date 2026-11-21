# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")
print("0..24:", obj[:24].hex(' '))

# 规范示例: 首字段 RS length (2字节 LE), 再 BS type
for start in (0, 4):
    bs = BitStream(obj); bs.byte=start
    size = bs.read_rs(); typ = bs.read_bs()
    print(f"@{start} RSsize={size} BStype={typ} cursor={bs.byte}.{bs.bit}")

# 首对象句柄应=1；RS 后 H(code=1,size=1 -> 字节01)
