# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")
print("head RL", hex(struct.unpack_from("<I", obj, 0)[0]))

def probe(start, label):
    bs = BitStream(obj); bs.byte = start; bs.bit = 0
    rl = bs.read_rl(); typ = bs.read_bs()
    print(f"{label} @{start}: RL={rl} BStype={typ} end={bs.byte}.{bs.bit}")

probe(4, "RL/BS")
probe(4, "MS/BS")
# 直接字节观察 4..20
print("4..20:", obj[4:20].hex(' '))
# 若首字段是 MS：从 4 读 MS
bs = BitStream(obj); bs.byte = 4
print("MS@4=", bs.read_ms(), "end", bs.byte, bs.bit)
