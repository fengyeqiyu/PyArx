"""柜体 = 共用完全相同 x0/x1 的一组水平行线。精确键分组，段内 y 极值即柜体真实上下界。"""
import json
import os
from collections import defaultdict

SCENE = r"G:\PyArx\viewer\web\scene.json"
OUT = r"G:\PyArx\viewer\web\cuts.html"
YMIN = 500000

d = json.load(open(SCENE, encoding="utf-8"))
L = d["ln"]
H = []
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if abs(y1 - y2) <= 40 and abs(x1 - x2) >= 3000:
        H.append((round(min(x1, x2)), round(max(x1, x2)), round((y1 + y2) / 2)))
print("水平线:", len(H))

g = defaultdict(list)
for x0, x1, y in H:
    g[(x0, x1)].append(y)

segs = []
for (x0, x1), ys in g.items():
    ys.sort()
    cur = [ys[0]]
    for y in ys[1:]:
        if y - cur[-1] > 5000:
            if len(cur) >= 3:
                segs.append({"x0": x0, "x1": x1, "y0": cur[0], "y1": cur[-1], "n": len(cur)})
            cur = [y]
        else:
            cur.append(y)
    if len(cur) >= 3:
        segs.append({"x0": x0, "x1": x1, "y0": cur[0], "y1": cur[-1], "n": len(cur)})

print("行线组(>=3行):", len(segs))
cab = [s for s in segs
       if 17000 <= s["x1"] - s["x0"] <= 36000 and 27000 <= s["y1"] - s["y0"] <= 54000 and s["n"] >= 5]
print("柜体量级组:", len(cab))
ws = sorted(s["x1"] - s["x0"] for s in cab)
hs = sorted(s["y1"] - s["y0"] for s in cab)
ns = sorted(s["n"] for s in cab)
if ws:
    print("宽 中位 %d [%d~%d]" % (ws[len(ws) // 2], ws[0], ws[-1]))
    print("高 中位 %d [%d~%d]" % (hs[len(hs) // 2], hs[0], hs[-1]))
    print("行数 中位 %d [%d~%d]" % (ns[len(ns) // 2], ns[0], ns[-1]))
cab.sort(key=lambda s: (-s["y1"], s["x0"]))
for s in cab[:10]:
    print("  %7d..%7d  %7d..%7d  %5dx%5d  %2d行"
          % (s["x0"], s["x1"], s["y0"], s["y1"], s["x1"] - s["x0"], s["y1"] - s["y0"], s["n"]))
json.dump(cab, open(r"G:\PyArx\viewer\export\cab_rows.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
