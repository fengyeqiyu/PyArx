"""柜框若为闭合多段线：找出轴对齐矩形，再与 79 张参数表配对。"""
import json
from collections import Counter

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))

rects = []
for p in d["pl"]:
    raw = p["p"]
    n = len(raw) // 2
    if n < 4:
        continue
    pts = [(raw[2 * i], raw[2 * i + 1]) for i in range(n)]
    xs = [q[0] for q in pts]
    ys = [q[1] for q in pts]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    w, h = x1 - x0, y1 - y0
    if w < 3000 or h < 3000:
        continue
    # 轴对齐：所有点都落在外接矩形的四条边上
    onedge = all(abs(qx - x0) < 1 or abs(qx - x1) < 1 or abs(qy - y0) < 1 or abs(qy - y1) < 1
                 for qx, qy in pts)
    if not onedge:
        continue
    rects.append({"b": [round(x0), round(y0), round(x1), round(y1)], "w": round(w), "h": round(h),
                  "n": n, "z": bool(p.get("z"))})

print("轴对齐矩形多段线:", len(rects))
if rects:
    print("宽高分布（取整千）:", Counter((r["w"] // 1000 * 1000, r["h"] // 1000 * 1000) for r in rects).most_common(10))

cand = [r for r in rects if 15000 <= r["w"] <= 45000 and 25000 <= r["h"] <= 70000]
print("\n柜体量级矩形:", len(cand))

found, missed = [], []
for i, tb in enumerate(d["tb"]):
    bb = tb["b"]
    cx, cy = (bb[0] + bb[2]) / 2, (bb[1] + bb[3]) / 2
    hit = [r for r in cand
           if r["b"][0] - 2000 <= cx <= r["b"][2] + 2000 and r["b"][1] - 2000 <= cy <= r["b"][3] + 2000]
    if hit:
        hit.sort(key=lambda r: r["w"] * r["h"])
        found.append((i, hit[0]["b"], len(hit)))
    else:
        missed.append((i, [round(v) for v in bb]))

print("匹配:", len(found), " 未匹配:", len(missed))
if found:
    ws = sorted(r[1][2] - r[1][0] for r in found)
    hs = sorted(r[1][3] - r[1][1] for r in found)
    print("框宽 中位 %.0f [%.0f~%.0f]  框高 中位 %.0f [%.0f~%.0f]"
          % (ws[len(ws) // 2], ws[0], ws[-1], hs[len(hs) // 2], hs[0], hs[-1]))
    print("\n前 12 个:")
    for i, box, n in found[:12]:
        print("  #%-3d %s  %.0fx%.0f 候选%d" % (i, box, box[2] - box[0], box[3] - box[1], n))
if missed:
    print("未匹配:", missed[:5])
json.dump({"found": [[i, b] for i, b, _ in found], "missed": missed},
          open(r"G:\PyArx\viewer\export\cab_rects.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
