import json
import re
from collections import Counter

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))
ly = d.get("ly", [])

# 柜名形如 101-1-1RCUAP1：三段且第二段是纯数字，与回路号 101-1BL1-WL9 区分开
CAB = re.compile(r"^\d{3}-\d-\d+[A-Za-z]")
LOOP = re.compile(r"^\d{3}-\d[A-Za-z0-9]+-")

cab, loop, other = [], 0, Counter()
for t in d.get("tx", []):
    s = (t[6] or "").strip()
    if not s or len(s) > 26:
        continue
    if CAB.match(s):
        cab.append((s, t[0], t[1], t[2], ly[t[7]] if 0 <= t[7] < len(ly) else "?"))
    elif LOOP.match(s):
        loop += 1
    else:
        other[s[:4]] += 1

print("柜名候选:", len(cab))
print("回路号:", loop)
cab.sort(key=lambda r: (round(r[2] / 1000), r[1]))
print("\n按平面位置排序：")
for s, x, y, h, l in cab:
    print("  %-22s x=%-10.0f y=%-9.0f h=%-7.0f %s" % (s, x, y, h, l))

hs = Counter(round(h) for _, _, _, h, _ in cab)
print("\n字高分布:", hs.most_common(8))
ys = Counter(round(y / 10000) for _, _, y, _, _ in cab)
print("y 万位分布:", sorted(ys.items()))
