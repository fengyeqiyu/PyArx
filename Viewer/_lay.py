# -*- coding: utf-8 -*-
import json
import sys

sys.path.insert(0, r"G:\PyArx\examples\cad2d")
import scan_load  # noqa: E402

DW = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.dwg"
d = json.load(open(scan_load._cache_path(DW), encoding="utf-8"))
lc = d.get("layer_col") or {}
print("layers", len(lc))
print("axis_keys", [k for k in lc if "AXIS" in k][:6])
print("sample", list(lc.items())[:6])
print("layers_tbl", list((d.get("layers") or {}).keys())[:8])
