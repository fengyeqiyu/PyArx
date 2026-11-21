# -*- coding: utf-8 -*-
import json
j = json.load(open(r"G:\PyArx\examples\cad2d\dwg1018\structure.json", encoding="utf-8"))
print("sections:", len(j["sections"]))
for s in j["sections"]:
    print(f"  id={s['section_id']:3d} pages={s['page_count']} comp={s['compressed']} "
          f"size={s['size']:7d} {s['name']}")
