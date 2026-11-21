# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)

for name, s in c.sections.items():
    if name in ("AcDb:Handles", "AcDb:AcDbObjects", "AcDb:Classs", "AcDb:AuxHeader"):
        print(f"== {name} id={s.section_id} size={s.size} pages={s.page_count} "
              f"max={s.max_page_size:#x} comp={s.compressed}")
        for pg in s.pages[:6]:
            print(f"   page_id={pg['page_id']} data_size={pg['data_size']} "
                  f"start_offset={pg['start_offset']:#x}")
        if len(s.pages) > 6:
            print(f"   ... ({len(s.pages) - 6} more)")

print("\npage_map entries:", len(c.page_map))
