"""在系统图带做密度连通聚类，看能否一簇一台柜；同时核对每簇里有没有柜名、参数表。"""
import json
import re
from collections import defaultdict

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))
ly = d.get("ly", [])

CELL = 6000
YMIN, YMAX = 550000, 830000

pts = []          # (x, y, kind, text)
L = d["ln"]
for i in range(0, len(L) - 5, 6):
    pts.append((L[i], L[i + 1], "l", ""))
    pts.append((L[i + 2], L[i + 3], "l", ""))
for p in d["pl"]:
    raw = p["p"]
    for i in range(0, len(raw) - 1, 2):
        pts.append((raw[i], raw[i + 1], "p", ""))
C = d["cr"]
for i in range(0, len(C) - 4, 5):
    pts.append((C[i], C[i + 1], "c", ""))
for t in d["tx"]:
    pts.append((t[0], t[1], "t", (t[6] or "").strip()))
for b in d["ins"]:
    pts.append((b["x"], b["y"], "i", b.get("k") or ""))
for tb in d["tb"]:
    bb = tb["b"]
    pts.append(((bb[0] + bb[2]) / 2, (bb[1] + bb[3]) / 2, "b", "TABLE"))

pts = [q for q in pts if q[0] and q[1] and YMIN <= q[1] <= YMAX]
print("系统图带图元点:", len(pts))

cells = defaultdict(list)
for q in pts:
    cells[(int(q[0] // CELL), int(q[1] // CELL))].append(q)

seen, groups = set(), []
for c in list(cells):
    if c in seen:
        continue
    bag, stack = [], [c]
    seen.add(c)
    while stack:
        cur = stack.pop()
        bag.extend(cells[cur])
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                nb = (cur[0] + dx, cur[1] + dy)
                if nb in cells and nb not in seen:
                    seen.add(nb)
                    stack.append(nb)
    groups.append(bag)

groups = [g for g in groups if len(g) >= 30]
print("簇数:", len(groups))

CAB = re.compile(r"^\d{3}-\d-\d+[A-Za-z]")
rows = []
for g in groups:
    xs = [q[0] for q in g]
    ys = [q[1] for q in g]
    w, hh = max(xs) - min(xs), max(ys) - min(ys)
    texts = [q[3] for q in g if q[2] == "t" and q[3]]
    has_pe = any(s.startswith("Pe=") for s in texts)
    has_ij = any(s.startswith("Ijs=") for s in texts)
    cabs = [s for s in texts if CAB.match(s) and ":" not in s and len(s) <= 20]
    rows.append((min(xs), min(ys), w, hh, len(g), len(texts), has_pe, has_ij, cabs[:3]))

rows.sort(key=lambda r: (round(r[1] / 20000), r[0]))
print("\nx0 / y0 / 宽 / 高 / 点数 / 文字 / Pe / Ijs / 柜名候选")
for r in rows[:40]:
    print("  %8.0f %8.0f %7.0f %7.0f %6d %4d %-5s %-5s %s"
          % (r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8]))
print("\n有 Pe 的簇:", sum(1 for r in rows if r[6]), "/ 有 Ijs:", sum(1 for r in rows if r[7]))
