# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
for name, s in c.sections.items():
    if "Objects" in name:
        sec = s
print("section:", sec.name, "id", sec.section_id, "size", sec.size,
      "pages", sec.page_count, "max_page", sec.max_page_size, "comp", sec.compressed)
for p in sec.pages:
    print("  page", p)
obj = c.read_section(sec.name)
print("len", len(obj))
for i in range(0, 80, 16):
    print(f"{i:5d}: {obj[i:i+16].hex(' ')}")
