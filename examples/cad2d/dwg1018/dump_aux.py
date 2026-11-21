# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
aux = c.read_section("AcDb:AuxHeader")
print("len", len(aux))
for i in range(0, len(aux), 16):
    print(f"{i:4d}: {aux[i:i+16].hex(' ')}")
