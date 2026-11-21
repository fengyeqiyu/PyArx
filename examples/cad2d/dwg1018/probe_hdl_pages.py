# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
for name, s in c.sections.items():
    if "Handles" in name or "Objects" in name:
        print(name, "pages=", s.page_count, "max=", s.max_page_size, "size=", s.size)
        for p in s.pages[:6]:
            print("   pid", p["page_id"], "data", p["data_size"], "off", p["start_offset"])
