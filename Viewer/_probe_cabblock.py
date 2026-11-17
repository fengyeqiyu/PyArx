"""检查 block_defs 是否带几何；若有，用插入矩阵算出每个柜体块的世界 bbox。"""
import glob
import json
import os
import tempfile
from collections import Counter

cache = os.path.join(tempfile.gettempdir(), "pyarx_scan")
p = sorted(glob.glob(os.path.join(cache, "*101*.json")), key=os.path.getmtime, reverse=True)[0]
s = json.load(open(p, encoding="utf-8"))
dd = s["block_defs"]
blocks = s["blocks"]

use = Counter(b.get("n") for b in blocks)
print("块名用量 top12:", use.most_common(12))

nonempty = [(k, v["geom"]) for k, v in dd.items()
            if v.get("geom") and any(v["geom"][kk] for kk in v["geom"])]
print("\n带几何的块定义:", len(nonempty), "/", len(dd))
if nonempty:
    k0, g0 = nonempty[0]
    print("示例", k0, {kk: (len(vv) if isinstance(vv, list) else vv) for kk, vv in g0.items()})


def bbox_of(g):
    xs, ys = [], []

    def add(x, y):
        if isinstance(x, (int, float)) and isinstance(y, (int, float)):
            xs.append(float(x))
            ys.append(float(y))

    for kk, vv in g.items():
        if not isinstance(vv, list):
            continue
        if kk == "lines":
            for i in range(0, len(vv) - 3, 4):
                add(vv[i], vv[i + 1])
                add(vv[i + 2], vv[i + 3])
        elif kk == "plines":
            for it in vv:
                raw = it.get("p") if isinstance(it, dict) else it
                if isinstance(raw, list) and raw and isinstance(raw[0], (int, float)):
                    for i in range(0, len(raw) - 1, 2):
                        add(raw[i], raw[i + 1])
                elif isinstance(raw, list):
                    for q in raw:
                        if isinstance(q, (list, tuple)) and len(q) >= 2:
                            add(q[0], q[1])
        elif kk == "circles":
            for it in vv:
                if isinstance(it, (list, tuple)) and len(it) >= 3:
                    add(it[0] - it[2], it[1] - it[2])
                    add(it[0] + it[2], it[1] + it[2])
                elif isinstance(it, dict) and "c" in it:
                    r = it.get("r", 0)
                    add(it["c"][0] - r, it["c"][1] - r)
                    add(it["c"][0] + r, it["c"][1] + r)
        elif kk == "arcs":
            for it in vv:
                if isinstance(it, dict) and "c" in it:
                    r = it.get("r", 0)
                    add(it["c"][0] - r, it["c"][1] - r)
                    add(it["c"][0] + r, it["c"][1] + r)
        elif kk == "texts":
            for it in vv:
                if isinstance(it, dict):
                    add(it.get("x"), it.get("y"))
                elif isinstance(it, (list, tuple)) and len(it) >= 2:
                    add(it[0], it[1])
    return [min(xs), min(ys), max(xs), max(ys)] if xs else None


defs = {}
for k, g in nonempty:
    bb = bbox_of(g)
    if bb:
        defs[k] = (bb, bb[2] - bb[0], bb[3] - bb[1])
print("可算 bbox 的块定义:", len(defs))

cand = []
for b in blocks:
    nm = b.get("n")
    if nm not in defs:
        continue
    bb, w, h = defs[nm]
    m = b.get("m") or [1, 0, 0, 1, 0, 0]
    m00, m10, m01, m11, tx, ty = m
    pts = [(m00 * x + m01 * y + tx, m10 * x + m11 * y + ty)
           for x, y in ((bb[0], bb[1]), (bb[2], bb[1]), (bb[2], bb[3]), (bb[0], bb[3]))]
    xs = [q[0] for q in pts]
    ys = [q[1] for q in pts]
    W, Hh = max(xs) - min(xs), max(ys) - min(ys)
    if 15000 <= W <= 45000 and 25000 <= Hh <= 70000:
        cand.append((nm, round(W), round(Hh), [round(min(xs)), round(min(ys)), round(max(xs)), round(max(ys))]))

print("\n柜体量级块参照:", len(cand))
c = Counter(r[0] for r in cand)
for nm, n in c.most_common(8):
    ws = [r[1] for r in cand if r[0] == nm]
    hs = [r[2] for r in cand if r[0] == nm]
    print("  %-28s %3d 次  %.0f~%.0f x %.0f~%.0f" % (nm[:28], n, min(ws), max(ws), min(hs), max(hs)))
for r in cand[:8]:
    print("   ", r[0][:20], r[1], "x", r[2], r[3])
