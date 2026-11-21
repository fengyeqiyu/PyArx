# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")
print("objects buffer:", len(obj))
for i in range(0, 96, 16):
    print(f"{i:4d}: {obj[i:i+16].hex(' ')}")

# 首对象: bit_read_MS (对象尺寸) — 先看 MS 定义
bs = BitStream(obj)
print("\nfirst byte:", hex(bs.read_rc()))
