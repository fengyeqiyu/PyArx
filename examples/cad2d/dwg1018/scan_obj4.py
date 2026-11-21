# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")

# 字节边界：RL size 后 BS type；也试 MS size 后 BS
for a in range(0, 24):
    for bit in range(0, 8):
        for kind in ("RL", "MS"):
            bs = BitStream(obj)
            bs.byte = a; bs.bit = bit
            try:
                size = bs.read_rl() if kind == "RL" else bs.read_ms()
                typ = bs.read_bs()
                if 4 <= size <= 600 and 0 <= typ <= 400:
                    print(f"@{a}.{bit} {kind}={size} BS={typ} end={bs.byte}.{bs.bit}")
            except Exception:
                pass
