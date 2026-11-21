# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
for name, s in c.sections.items():
    ds = [p["data_size"] for p in s.pages]
    mx = max(ds) if ds else 0
    print(f"{name:24s} id={s.section_id:2d} pages={s.page_count:3d} max_page={s.max_page_size:6d} max_data_size={mx:6d} size={s.size}")
