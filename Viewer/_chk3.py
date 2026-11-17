# -*- coding: utf-8 -*-
"""解析 scene html 内嵌数据，统计文字条目并抽样轴号。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
m = re.search(r"var\s+(SCENE|DATA|S)\s*=\s*", s)
print("var:", m.group(1) if m else None)
if not m:
    idx = s.find('"tx":')
    print("tx idx", idx, s[idx:idx + 120])
    raise SystemExit(0)
start = m.end()
# 从 start 开始做括号配平
depth = 0
i = start
while i < len(s):
    ch = s[i]
    if ch == "{":
        depth += 1
    elif ch == "}":
        depth -= 1
        if depth == 0:
            break
    i += 1
raw = s[start:i + 1]
raw = re.sub(r"(?<![:\[,{])(\w+):", r'"\1":', raw)
data = json.loads(raw)
print("keys", list(data.keys())[:12])
tx = data.get("tx") or []
print("tx", len(tx))
axis = [t for t in tx if str(t[6]).strip() in ("1", "2", "3", "4", "5", "6", "7", "8")]
print("axis-like", len(axis))
for t in axis[:8]:
    print("  ", t)
for t in tx[:6]:
    print("  s", t)
