# -*- coding: utf-8 -*-
"""统计 scene tx 里的越界文字与正常文字数量。"""
import json
import re

P = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(P, encoding="utf-8", errors="ignore").read()
m = re.search(r'<script id="scene"[^>]*>(.*?)</script>', s, re.S)
data = json.loads(m.group(1).strip())
tx = data.get("tx") or []
b = data["b"]
print("tx total", len(tx))
print("b", [round(v) for v in b])

# 图框范围（cb 更宽）
cb = data["cb"]
inside = 0
outliers = []
for t in tx:
    x, y = float(t[0]), float(t[1])
    if cb[0] - 1 <= x <= cb[2] + 1 and cb[1] - 1 <= y <= cb[3] + 1:
        inside += 1
    else:
        outliers.append(t)
print("inside cb", inside, "outside cb", len(outliers))
for t in outliers[:15]:
    print("  OUT", [round(float(t[0])), round(float(t[1])), t[2], t[6] if len(t) > 6 else ""][:4])
# x 分布极值
xs = [float(t[0]) for t in tx]
ys = [float(t[1]) for t in tx]
print("x range %.0f .. %.0f" % (min(xs), max(xs)))
print("y range %.0f .. %.0f" % (min(ys), max(ys)))
