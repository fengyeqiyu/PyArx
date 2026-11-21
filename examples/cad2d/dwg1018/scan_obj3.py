# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")

# 每个字节偏移、每个 bit 偏移，试 MS -> BS
for a in range(0, 40):
    for bit in range(0, 8):
        bs = BitStream(obj)
        bs.byte = a; bs.bit = bit
        try:
            size = bs.read_ms()
            typ = bs.read_bs()
            if 4 <= size <= 4000 and 0 <= typ <= 400:
                print(f"@{a}.{bit}: MS={size} BS={typ} end={bs.byte}.{bs.bit}")
        except Exception:
            pass
