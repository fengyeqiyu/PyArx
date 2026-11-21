# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:Handles"]
print("size", s.size, "pages", s.page_count, "max", s.max_page_size)
for p in s.pages[:4]:
    print(" ", p)
hdl = c.read_section(s.name)
print("hdl decomp len", len(hdl))
print("head", hdl[:12].hex(' '))
# 页内：RS_BE section_size 在 bit0；body 到 start+section_size；CRC RS_BE
