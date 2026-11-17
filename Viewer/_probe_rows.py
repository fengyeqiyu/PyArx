"""柜体由一组同 x 范围、等距的水平行线构成：按行线分组得到每台柜的真实范围。"""
import json
from collections import defaultdict

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))

H = []
L = d["ln"]
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if abs(y1 - y2) <= 40 and abs(x1 - x2) >= 3000:
        H.append((round(min(x1, x2)), round(max(x1, x2)), round((y1 + y2) / 2)))

print("水平线(>=3000):", len(H))

# 按 (x0,x1) 归并
groups = defaultdict(list)
for x0, x1, y in H:
    groups[(round(x0 / 600), round(x1 / 600))].append((x0, x1, y))

merged = []
for k, items in groups.items():
    xs = [i[0] for i in items]
    xe = [i[1] for i in items]
    ys = sorted(i[2] for i in items)
    merged.append({"x0": min(xs), "x1": max(xe), "ys": ys, "n": len(items)})

# 组内按 y 连续性切成段
segs = []
for g in merged:
    cur = [g["ys"][0]]
    for y in g["ys"][1:]:
        if y - cur[-1] > 4000:
            segs.append({"x0": g["x0"], "x1": g["x1"], "y0": cur[-1], "y1": cur[0], "n": len(cur)})
            cur = []
        cur.append(y)
    segs.append({"x0": g["x0"], "x1": g["x1"], "y0": cur[-1], "y1": cur[0], "n": len(cur)})

segs = [s for s in segs if s["n"] >= 4]
print("行线段:", len(segs))
cab = [s for s in segs if 18000 <= s["x1"] - s["x0"] <= 40000 and 25000 <= s["y1"] - s["y0"] <= 60000]
print("柜体量级段:", len(cab))

cab.sort(key=lambda s: (-s["y1"], s["x0"]))
print("\nx0..x1 / y0..y1 / 宽 / 高 / 行数")
for s in cab[:20]:
    print("  %7d..%7d  %7d..%7d  %6d x %6d  %3d 行"
          % (s["x0"], s["x1"], s["y0"], s["y1"], s["x1"] - s["x0"], s["y1"] - s["y0"], s["n"]))
ws = sorted(s["x1"] - s["x0"] for s in cab)
hs = sorted(s["y1"] - s["y0"] for s in cab)
if ws:
    print("\n宽 中位 %d [%d~%d]   高 中位 %d [%d~%d]"
          % (ws[len(ws) // 2], ws[0], ws[-1], hs[len(hs) // 2], hs[0], hs[-1]))

json.dump(cab, open(r"G:\PyArx\viewer\export\cab_rows.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("写出 export\\cab_rows.json")
