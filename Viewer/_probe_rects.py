"""在每张参数表附近找由 4 条线构成的闭合矩形（柜体外框），作为该柜的真实边界。"""
import json

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))
L = d["ln"]
NS = 6

H, V = [], []
for i in range(0, len(L) - 5, NS):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if abs(y1 - y2) <= 40 and abs(x1 - x2) >= 10000:
        H.append((min(x1, x2), max(x1, x2), (y1 + y2) / 2))
    elif abs(x1 - x2) <= 40 and abs(y1 - y2) >= 10000:
        V.append((min(y1, y2), max(y1, y2), (x1 + x2) / 2))

print("水平长线:", len(H), " 垂直长线:", len(V))

tables = [(tb["b"], i) for i, tb in enumerate(d["tb"])]
found, missed = [], []
for bb, ti in tables:
    tx0, ty0, tx1, ty1 = bb[0], bb[1], bb[2], bb[3]
    tcx, tcy = (tx0 + tx1) / 2, (ty0 + ty1) / 2
    cand = []
    for bot in H:
        if not (ty0 - 55000 <= bot[2] <= ty1 + 5000):
            continue
        for top in H:
            if top[2] <= bot[2] or not (ty1 <= top[2] <= ty1 + 55000):
                continue
            if abs(top[0] - bot[0]) > 800 or abs(top[1] - bot[1]) > 800:
                continue
            xl, xr = (top[0] + bot[0]) / 2, (top[1] + bot[1]) / 2
            if not (xl - 3000 <= tcx <= xr + 3000):
                continue
            w, h = xr - xl, top[2] - bot[2]
            if not (14000 <= w <= 45000 and 25000 <= h <= 70000):
                continue
            # 左右两条竖线要接上
            vl = [v for v in V if abs(v[2] - xl) < 800 and v[0] <= bot[2] + 800 and v[1] >= top[2] - 800]
            vr = [v for v in V if abs(v[2] - xr) < 800 and v[0] <= bot[2] + 800 and v[1] >= top[2] - 800]
            if vl and vr:
                cand.append([round(xl), round(bot[2]), round(xr), round(top[2])])
    if cand:
        cand.sort(key=lambda c: (c[2] - c[0]) * (c[3] - c[1]))
        found.append((ti, cand[0], len(cand)))
    else:
        missed.append((ti, [round(v) for v in bb]))

print("\n匹配到柜框:", len(found), " 未匹配:", len(missed))
ws = [c[0][2] - c[0][0] for c in found]
hs = [c[0][3] - c[0][1] for c in found]
if ws:
    ws.sort()
    hs.sort()
    print("框宽 中位 %.0f  [%.0f ~ %.0f]" % (ws[len(ws) // 2], ws[0], ws[-1]))
    print("框高 中位 %.0f  [%.0f ~ %.0f]" % (hs[len(hs) // 2], hs[0], hs[-1]))
print("\n前 14 个：")
for ti, box, n in found[:14]:
    print("  #%-3d %s  宽%.0f 高%.0f 候选%d" % (ti, box, box[2] - box[0], box[3] - box[1], n))
if missed:
    print("\n未匹配示例:", missed[:6])
json.dump({"found": [[ti, box] for ti, box, _ in found], "missed": missed},
          open(r"G:\PyArx\viewer\export\cab_rects.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
