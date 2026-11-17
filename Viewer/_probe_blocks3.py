import glob
import json
import os
import tempfile
from collections import Counter

cache = os.path.join(tempfile.gettempdir(), "pyarx_scan")
p = sorted(glob.glob(os.path.join(cache, "*101*.json")), key=os.path.getmtime, reverse=True)[0]
s = json.load(open(p, encoding="utf-8"))
dd = s["block_defs"]

for name in ("AR3_INFO",):
    g = dd[name]["geom"]
    print(name, "geom keys:", list(g.keys()))
    for k, v in g.items():
        if isinstance(v, list):
            print("   ", k, "len", len(v), "sample", json.dumps(v[:2], ensure_ascii=False)[:160])
        else:
            print("   ", k, type(v).__name__, str(v)[:80])


def def_bbox(g):
    xs, ys = [], []

    def add(x, y):
        xs.append(x)
        ys.append(y)

    for k, v in g.items():
        if not isinstance(v, list):
            continue
        if k in ("lines", "ln"):
            for i in range(0, len(v) - 3, 4):
                add(v[i], v[i + 1])
                add(v[i + 2], v[i + 3])
        elif k in ("plines", "pl"):
            for it in v:
                raw = it.get("p") if isinstance(it, dict) else it
                if isinstance(raw, list):
                    for i in range(0, len(raw) - 1, 2):
                        if isinstance(raw[i], (int, float)):
                            add(raw[i], raw[i + 1])
        elif k in ("circles", "cr"):
            for it in v:
                if isinstance(it, (list, tuple)) and len(it) >= 3:
                    add(it[0] - it[2], it[1] - it[2])
                    add(it[0] + it[2], it[1] + it[2])
        elif k in ("arcs", "ar"):
            for it in v:
                if isinstance(it, dict) and "c" in it:
                    c = it["c"]
                    add(c[0] - it["r"], c[1] - it["r"])
                    add(c[0] + it["r"], c[1] + it["r"])
        elif k in ("texts", "tx"):
            for it in v:
                if isinstance(it, dict):
                    add(it.get("x", 0), it.get("y", 0))
    if not xs:
        return None
    return [min(xs), min(ys), max(xs), max(ys)]


sizes = []
for name, bd in dd.items():
    bb = def_bbox(bd.get("geom") or {})
    if bb:
        sizes.append((name, bb[2] - bb[0], bb[3] - bb[1]))

print("\n有几何的块定义:", len(sizes))
sizes.sort(key=lambda r: -(r[1] * r[2]))
print("最大的 12 个（定义内尺寸）：")
for name, w, h in sizes[:12]:
    print("  %-24s %8.0f x %8.0f" % (name[:24], w, h))

# 用插入矩阵算世界 bbox
cand = []
for b in s["blocks"]:
    name = b.get("n")
    bb = None
    for nm, w, h in sizes:
        if nm == name:
            break
    g = dd.get(name, {}).get("geom")
    if not g:
        continue
    if bb is None:
        bb = def_bbox(g)
    if not bb:
        continue
    m = b.get("m") or [1, 0, 0, 1, 0, 0]
    m00, m10, m01, m11, tx, ty = m
    pts = []
    for x, y in ((bb[0], bb[1]), (bb[2], bb[1]), (bb[2], bb[3]), (bb[0], bb[3])):
        pts.append((m00 * x + m01 * y + tx, m10 * x + m11 * y + ty))
    xs = [q[0] for q in pts]
    ys = [q[1] for q in pts]
    w, h = max(xs) - min(xs), max(ys) - min(ys)
    if 15000 <= w <= 45000 and 25000 <= h <= 70000:
        cand.append((name, round(w), round(h), [round(min(xs)), round(min(ys)), round(max(xs)), round(max(ys))]))

print("\n柜体量级块参照:", len(cand))
cnt = Counter(r[0] for r in cand)
for nm, c in cnt.most_common(12):
    ws = [r[1] for r in cand if r[0] == nm]
    hs = [r[2] for r in cand if r[0] == nm]
    print("  %-24s %3d 次  %.0f~%.0f x %.0f~%.0f" % (nm[:24], c, min(ws), max(ws), min(hs), max(hs)))
print("\n前 12 个：")
for r in cand[:12]:
    print("  %-20s %.0fx%.0f %s" % (str(r[0])[:20], r[1], r[2], r[3]))
