# -*- coding: utf-8 -*-
"""重扫 101：验证 cast 到真实类后能否读出轴号文字。"""
import json
import os
import sys

sys.path.insert(0, r"G:\PyArx\examples\cad2d")
import scan_load  # noqa: E402

DW = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.dwg"
p = scan_load._cache_path(DW)
if os.path.isfile(p):
    os.remove(p)
scan_load.scan_dwg(DW, use_cache=False)
d = json.load(open(p, encoding="utf-8"))

for line in d["err"]:
    if line.startswith(("APP", "XD2")):
        print("DBG", line)
for nm, v in d["block_defs"].items():
    if "AXISO" in nm:
        g = v.get("geom") or {}
        print("def", nm[-18:], "n", v.get("n"), "types", v.get("types"), "texts", g.get("texts"))
