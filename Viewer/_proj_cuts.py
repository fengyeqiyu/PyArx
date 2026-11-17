"""投影法切柜：把系统图带矢量图元栅格化，按行/列黑点投影找空白缝，缝间即一台柜。"""
import json
import numpy as np

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))
YMIN = 500000
CELL = 80

L = d["ln"]
xs = [L[i] for i in range(0, len(L), 6)] + [L[i + 2] for i in range(0, len(L), 6)]
ys = [L[i + 1] for i in range(0, len(L), 6)] + [L[i + 3] for i in range(0, len(L), 6)]
X0, X1 = min(xs), max(xs)
YLO, YHI = YMIN, max(ys)
COLS = int((X1 - X0) / CELL) + 2
ROWS = int((YHI - YLO) / CELL) + 2
print("栅格 %d x %d (cell=%d)" % (COLS, ROWS, CELL))

G = np.zeros((ROWS, COLS), np.uint8)


def put(x, y):
    c = int((x - X0) / CELL)
    r = int((y - YLO) / CELL)
    if 0 <= r < ROWS and 0 <= c < COLS:
        G[r, c] = 1


for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if max(y1, y2) < YMIN:
        continue
    dx, dy = abs(x2 - x1), abs(y2 - y1)
    # 只保留柜框类轴对齐线：斜引线（设备连线）与超长跨区线会污染投影
    if dy <= 40 and 3000 <= dx <= 52000:
        pass
    elif dx <= 40 and 3000 <= dy <= 62000:
        pass
    else:
        continue
    n = max(2, int(max(dx, dy) / CELL))
    for t in range(n + 1):
        put(x1 + (x2 - x1) * t / n, y1 + (y2 - y1) * t / n)

for p in d["pl"]:
    raw = p["p"]
    if not raw or max(raw[1::2]) < YMIN:
        continue
    for j in range(0, len(raw) - 3, 2):
        x1, y1, x2, y2 = raw[j], raw[j + 1], raw[j + 2], raw[j + 3]
        if abs(x2 - x1) > 40 and abs(y2 - y1) > 40:
            continue
        n = max(2, int(max(abs(x2 - x1), abs(y2 - y1)) / CELL))
        for t in range(n + 1):
            put(x1 + (x2 - x1) * t / n, y1 + (y2 - y1) * t / n)

for i in range(0, len(d["cr"]) - 4, 5):
    if d["cr"][i + 1] >= YMIN:
        put(d["cr"][i], d["cr"][i + 1])

print("非空栅格:", int(G.sum()))


def gaps(proj, thr, minlen):
    """返回空白段 [(start,end)]，以及有内容段。"""
    blank = proj <= thr
    segs, s = [], None
    for i, b in enumerate(blank):
        if b and s is None:
            s = i
        elif not b and s is not None:
            if i - s >= minlen:
                segs.append((s, i))
            s = None
    if s is not None and len(blank) - s >= minlen:
        segs.append((s, len(blank)))
    return segs


row = G.sum(axis=1)
print("行投影 最大 %d 中位 %.0f" % (row.max(), np.median(row)))

for thr, minlen in ((0, 20), (0, 12), (1, 16)):
    rsegs = gaps(row, thr, minlen)
    print("\n[thr=%d minlen=%d] 水平缝 %d 条 -> 横带 %d" % (thr, minlen, len(rsegs), len(rsegs) + 1))
    bounds = [0] + [(a + b) // 2 for a, b in rsegs] + [ROWS - 1]
    bands = []
    for i in range(len(bounds) - 1):
        a, b = bounds[i], bounds[i + 1]
        if b - a < 60:
            continue
        bands.append((a, b))
        col = G[a:b].sum(axis=0)
        csegs = gaps(col, thr, minlen)
        cb = [0] + [(p + q) // 2 for p, q in csegs] + [COLS - 1]
        cells = [(cb[j], cb[j + 1]) for j in range(len(cb) - 1) if cb[j + 1] - cb[j] >= 60]
        wlist = " ".join(str((c1 - c0) * CELL) for c0, c1 in cells[:8])
        print("  横带 y=%d..%d (高%d) 竖切 %d 段  宽: %s" % (a, b, b - a, len(cells), wlist))
