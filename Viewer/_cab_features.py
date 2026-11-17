"""多角度交叉验证柜的边界特征：线 / 多段线 / 文字 / 块参照 四类图元各自量 bbox，
比对它们的四条边是否一致，从而判定哪些台可信、哪些台是框本身不准。"""
import json
import statistics

SCENE = r"G:\PyArx\viewer\web\scene.json"
CAB = r"G:\PyArx\viewer\export\cabinets_all.json"
YMIN = 500000
TOL = 4000          # 四类判据边界差 < TOL 视为一致

d = json.load(open(SCENE, encoding="utf-8"))
cabs = json.load(open(CAB, encoding="utf-8"))["cabinets"]

L = d["ln"]
lines = [(L[i], L[i + 1], L[i + 2], L[i + 3]) for i in range(0, len(L) - 5, 6)
         if max(L[i + 1], L[i + 3]) >= YMIN]
plines = []
for p in d["pl"]:
    raw = p["p"]
    if raw and max(raw[1::2]) >= YMIN:
        plines.append(raw)
texts = [(t[0], t[1], t[2], t[6]) for t in d["tx"] if t[1] >= YMIN and (t[6] or "").strip()]
blocks = [(b["x"], b["y"]) for b in d["ins"] if b.get("y", 0) >= YMIN]


def bbox_of(pts):
    if not pts:
        return None
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    return [min(xs), min(ys), max(xs), max(ys)]


def inside(x, y, b):
    return b[0] <= x <= b[2] and b[1] <= y <= b[3]


rows = []
for c in cabs:
    b = c["box"]
    lp = [(x1, y1) for x1, y1, x2, y2 in lines if inside((x1 + x2) / 2, (y1 + y2) / 2, b)]
    pp = [(raw[i], raw[i + 1]) for raw in plines
          for i in range(0, len(raw) - 1, 2) if inside(raw[i], raw[i + 1], b)]
    tp = [(t[0], t[1]) for t in texts if inside(t[0], t[1], b)]
    bp = [(x, y) for x, y in blocks if inside(x, y, b)]

    bb = {"line": bbox_of(lp), "pline": bbox_of(pp), "text": bbox_of(tp), "block": bbox_of(bp)}
    # 只取有足够样本的判据
    used = [k for k in bb if bb[k] and {"line": len(lp), "pline": len(pp), "text": len(tp), "block": len(bp)}[k] >= 8]
    edges = {"l": [], "r": [], "b": [], "t": []}
    for k in used:
        edges["l"].append(bb[k][0]); edges["r"].append(bb[k][2])
        edges["b"].append(bb[k][1]); edges["t"].append(bb[k][3])
    spread = {e: (max(v) - min(v)) if len(v) > 1 else 0 for e, v in edges.items()}
    boxw, boxh = b[2] - b[0], b[3] - b[1]
    # 框比实际内容宽多少
    content_r = min(edges["r"]) if edges["r"] else b[2]
    content_t = min(edges["t"]) if edges["t"] else b[3]
    rows.append({"i": c["i"] + 1, "name": c["name"], "pe": c["pe"], "used": len(used),
                 "spread": spread, "maxspread": max(spread.values()),
                 "boxw": boxw, "boxh": boxh,
                 "slack_r": boxw - (content_r - b[0]), "slack_t": boxh - (content_t - b[1]),
                 "n": {"line": len(lp), "pline": len(pp), "text": len(tp), "block": len(bp)}, "box": b})

ok = [r for r in rows if r["maxspread"] < TOL and r["used"] >= 3]
bad = [r for r in rows if r not in ok]
print("四判据一致 %d 台 / 不一致 %d 台（容差 %d）" % (len(ok), len(bad), TOL))
print("判据可用数分布:", {k: sum(1 for r in rows if r["used"] == k) for k in range(0, 5)})
w = sorted(r["boxw"] for r in rows)
h = sorted(r["boxh"] for r in rows)
print("框宽 中位 %d  四分位 %d~%d" % (w[len(w) // 2], w[len(w) // 4], w[3 * len(w) // 4]))
print("框高 中位 %d  四分位 %d~%d" % (h[len(h) // 2], h[len(h) // 4], h[3 * len(h) // 4]))
sr = sorted(r["slack_r"] for r in rows)
print("右侧空框量 中位 %d  最大 %d   >6000 的台数 %d" % (sr[len(sr) // 2], sr[-1], sum(1 for v in sr if v > 6000)))

print("\n不一致的台（各判据边界离散最大的边）:")
for r in bad[:18]:
    print("  #%-2d %-16s 判据%d 最大离散 %-6d %s  框 %dx%d"
          % (r["i"], (r["name"] or "—")[:16], r["used"], r["maxspread"],
             {k: v for k, v in r["spread"].items() if v == r["maxspread"]}, r["boxw"], r["boxh"]))
print("\n一致性最好的 5 台:")
for r in sorted(ok, key=lambda v: v["maxspread"])[:5]:
    print("  #%-2d %-16s 离散 %-5d 框 %dx%d"
          % (r["i"], (r["name"] or "—")[:16], r["maxspread"], r["boxw"], r["boxh"]))
json.dump(rows, open(r"G:\PyArx\viewer\export\cab_features.json", "w", encoding="utf-8"),
          ensure_ascii=False, indent=1)
print("\n写出 export\\cab_features.json")
