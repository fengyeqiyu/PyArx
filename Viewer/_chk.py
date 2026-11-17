# -*- coding: utf-8 -*-
"""检查 scan.json 的 attrib 采集结果。"""
import json

d = json.load(open(r"C:\temp\scan.json", encoding="utf-8"))
at = d.get("attrib") or []
print("attrib", len(at), "| counts", {k: v for k, v in (d.get("counts") or {}).items()
                                      if k in ("total", "INSERT", "TEXT", "attrib")})
for a in at[:12]:
    print("  ", a)
print("err", (d.get("err") or [])[-3:])
