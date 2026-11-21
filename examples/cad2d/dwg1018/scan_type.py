# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")

def valid(t):
    return t in (0,256) or 1 <= t <= 0x49 or t in (0x4f,0x50,0x51,0x52,0x1f2,0x1f3)

for a in range(0, 12):
    for bit in range(0, 8):
        bs = BitStream(obj); bs.byte=a; bs.bit=bit
        try:
            t = bs.read_bs()
            if valid(t):
                print(f"BS@{a}.{bit}={t} end={bs.byte}.{bs.bit}")
        except Exception:
            pass
