# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
cls = c.read_section("AcDb:Classs")
print("len", len(cls))
for i in range(0, 32, 16):
    print(f"{i:4d}: {cls[i:i+16].hex(' ')}")
print("page boundary 24320..24336:", cls[24320:24336].hex(' '))
