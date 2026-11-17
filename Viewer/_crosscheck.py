"""多特征交叉：柜名锚点(dx=2372,dy 变量,h=311) + 系统图$0$ 专用块 + 图层线，反推每台柜的真实边界。"""
import json
import re
from collections import Counter

SCENE = r"G:\PyArx\viewer\web\scene.json"
CAB = r"G:\PyArx\viewer\export\cabinets_all.json"
YMIN = 500000

d = json.load(open(SCENE, encoding="utf-8"))
cabs = json.load(open(CAB, encoding="utf-8"))["cabinets"]
NAME = re.compile(r"^\d{3}-\d+-\d{0,2}[A-Z]{2,}\d{0,2}(:\w+)?$")

ins = d["ins"]
tb = d["tb"]
txt = [(t[0], t[1], t[2], t[6]) for t in d["tx"] if t[1] >= YMIN and (t[6] or "").strip()]

print("=== A. 系统图$0$ 专用块普查 ===")
pref = [b for b in ins if str(b.get("k", "")).startswith("系统图$0$")]
print("  数量 %d，名单前12 %s" % (len(pref), Counter(b["k"] for b in pref).most_common(12)))
xs = sorted(b["x"] for b in pref)
ys = sorted(b["y"] for b in pref)
print("  块 x 范围 %d~%d，y 范围 %d~%d" % (xs[0], xs[-1], ys[0], ys[-1]))

print("\n=== B. 柜名锚点普查（格式 + 字高 + 相对表 dx）===")
names = [t for t in txt if NAME.match(t[3].strip()) and 250 <= t[2] <= 340]
print("  严格柜名 %d 个（字高 250~340）" % len(names))
gb = []
for n in names:
    best, bd = None, 1e18
    for t in tb:
        bb = t["b"]
        dx = n[0] - bb[0]
        if abs(dx - 2372) > 600:
            continue
        dy = n[1] - bb[1]
        if dy < 0:
            continue
        if dy < bd:
            bd, best = dy, bb
    gb.append((n, best, bd))
hit = [g for g in gb if g[1] is not None]
print("  能配上表(dx≈2372,表在上方 0~50000) 的 %d / %d" % (len(hit), len(names)))
dy = sorted(int(g[2]) for g in hit)
if dy:
    print("  配对后 dy 分布：中位 %d，最小 %d，最大 %d，四分位 %d/%d"
          % (dy[len(dy) // 2], dy[0], dy[-1], dy[len(dy) // 4], dy[3 * len(dy) // 4]))

print("\n=== C. 每台柜：块覆盖与线覆盖（决定边界）===")
okn = 0
rows = []
for n, bb, dyv in hit:
    if bb is None:
        continue
    x0, y0, x1, y1 = bb
    grp = []
    for b in pref:
        if x0 - 8000 <= b["x"] <= x0 + 34000 and y0 - 3000 <= b["y"] <= n[1] + 6000:
            grp.append(b)
    if grp:
        bx0 = min(b["x"] for b in grp); bx1 = max(b["x"] for b in grp)
        by0 = min(b["y"] for b in grp); by1 = max(b["y"] for b in grp)
        okn += 1
        rows.append((n[3], len(grp), bx1 - bx0, by1 - by0, bx0 - x0, bx1 - x0, by1 - y0, dyv))
print("  有柜内专用块组 %d / %d 台" % (okn, len(hit)))
if rows:
    w = sorted(r[2] for r in rows)
    h = sorted(r[3] for r in rows)
    print("  块组宽 中位 %d (%d~%d)，高 中位 %d (%d~%d)"
          % (w[len(w) // 2], w[0], w[-1], h[len(h) // 2], h[0], h[-1]))
    print("  块组相对表 x0 左偏中位 %d，右界中位 %d" % (sorted(r[4] for r in rows)[len(rows) // 2],
                                                    sorted(r[5] for r in rows)[len(rows) // 2]))
    for r in rows[:10]:
        print("   %-18s 块%2d 宽%6d 高%6d 表x0偏左%6d 右界x0+%6d 顶y0+%6d 名dy%6d" % r)

print("\n=== D. 交叉验证：块组右界 vs 当前框右界 ===")
cur = {c["name"]: c["box"] for c in cabs}
agree = 0
for r in rows:
    b = cur.get(r[0])
    if not b:
        continue
    dbr = (b[2] - (b[0] + r[5]))
    agree += 1 if abs(dbr) < 3000 else 0
print("  可比对 %d 台，框右界与块组右界相差 <3000 的 %d 台" % (len([r for r in rows if r[0] in cur]), agree))
