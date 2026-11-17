# -*- coding: utf-8 -*-
"""全局找“同一块在同一位置的多个实例”，以及位置几乎重合的实例对——这类会在画面上画重。"""
import json, collections

p = r"J:\变更图纸\DRB018\电照\101#中试车间(扩建)-设施配电平面及系统_t3.html"
s = open(p, encoding="utf-8").read()
a = s.find('id="scene"'); a = s.find(">", a) + 1; b = s.find("</script>", a)
D = json.loads(s[a:b])

ins = D["ins"]
print("实例总数", len(ins))

# 1) 完全同位置同块
c1 = collections.Counter((e["k"], round(e["x"], 2), round(e["y"], 2)) for e in ins)
d1 = [(v, k) for k, v in c1.items() if v > 1]
d1.sort(reverse=True)
print("完全同块同坐标的多实例: %d 组，涉及 %d 个实例" % (len(d1), sum(v for v, _ in d1)))
for v, k in d1[:10]:
    print("   x%d %-36s @(%.0f,%.0f)" % (v, k[0][:36], k[1], k[2]))

# 2) 同块、位置极近（<=3 单位）但不同坐标
g = collections.defaultdict(list)
for e in ins:
    g[e["k"]].append(e)
near = 0; shown = 0
print("\n同块位置几乎重合(<=3单位)的实例对:")
for k, lst in g.items():
    if len(lst) < 2:
        continue
    pts = sorted(((e["x"], e["y"]) for e in lst))
    for i in range(len(pts) - 1):
        if abs(pts[i][0] - pts[i+1][0]) <= 3 and abs(pts[i][1] - pts[i+1][1]) <= 3:
            near += 1
            if shown < 8:
                print("   %-34s (%.1f,%.1f) vs (%.1f,%.1f)" % (k[:34], pts[i][0], pts[i][1], pts[i+1][0], pts[i+1][1]))
                shown += 1
print("   合计 %d 对" % near)
