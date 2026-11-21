# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")
print("0..24:", obj[:24].hex(' '))

# @4 起，实体公共: MS size -> BS type
bs = BitStream(obj); bs.byte=4
size = bs.read_ms(); typ = bs.read_bs()
print(f"@4 MS={size} BStype={typ} cursor={bs.byte}.{bs.bit}")

# 对象公共(非实体): RL bitsize -> OT(BS)
bs = BitStream(obj); bs.byte=4
bitsize = bs.read_rl(); typ2 = bs.read_bs()
print(f"@4 RL={bitsize} BStype={typ2} cursor={bs.byte}.{bs.bit}")
