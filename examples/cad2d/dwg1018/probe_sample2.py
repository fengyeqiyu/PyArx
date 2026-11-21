# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
c=open_dwg(r"G:\PyArx\docs\libredwg_src\test\test-data\2004\circle.dwg")
for k,v in c.sections.items():
    print(k,"pages",v.page_count,"pids",[p["page_id"] for p in v.pages][:6])
print("page_map keys",sorted(c.page_map.keys())[:20])
