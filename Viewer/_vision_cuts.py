"""视觉算法识别柜体：
1) 矢量栅格化成二值图像（等价于渲染截图，但分辨率与坐标可控）
2) 形态学开运算抽出长横线/长竖线，滤掉文字笔画、斜引线
3) 长横线端点坐标一维聚类（容差 150），把同一条边上的线归到同一列位
4) 同列位、y 相邻（间隔 < 5000）的横线成组 —— 一组即一台柜
5) 组内 x 极值 / y 极值即柜体真实框
"""
import json
from collections import defaultdict

import numpy as np

SCENE = r"G:\PyArx\viewer\web\scene.json"
YMIN = 500000
CELL = 200          # 像素分辨率：1 px = 200 世界单位
H_MINLEN = 4500     # 长横线最小长度（世界单位）
V_MINLEN = 16000    # 长竖线最小长度
TOL = 150           # 端点聚类容差

d = json.load(open(SCENE, encoding="utf-8"))
L = d["ln"]

H = []
V = []
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if max(y1, y2) < YMIN:
        continue
    dx, dy = abs(x2 - x1), abs(y2 - y1)
    if dy <= 40 and dx >= H_MINLEN:
        H.append((round(min(x1, x2)), round(max(x1, x2)), round((y1 + y2) / 2)))
    elif dx <= 40 and dy >= V_MINLEN:
        V.append((round(min(y1, y2)), round(max(y1, y2)), round((x1 + x2) / 2)))

print("【步骤2】形态学筛选：长横线 %d 条，长竖线 %d 条" % (len(H), len(V)))


def cluster_1d(vals, tol=TOL):
    s = sorted(set(vals))
    out, cur = [], [s[0]]
    for v in s[1:]:
        if v - cur[-1] <= tol:
            cur.append(v)
        else:
            out.append(int(np.median(cur)))
            cur = [v]
    out.append(int(np.median(cur)))
    return out


def snap(v, centers, tol=TOL):
    best, bd = None, tol + 1
    for c in centers:
        dd = abs(c - v)
        if dd < bd:
            best, bd = c, dd
    return best


x0c = cluster_1d([h[0] for h in H])
x1c = cluster_1d([h[1] for h in H])
print("【步骤3】端点聚类：x 起点 %d 个列位，x 终点 %d 个列位" % (len(x0c), len(x1c)))

g = defaultdict(list)
unsnapped = 0
for x0, x1, y in H:
    s0 = snap(x0, x0c)
    s1 = snap(x1, x1c)
    if s0 is None or s1 is None:
        unsnapped += 1
        continue
    g[(s0, s1)].append(y)
print("       未能吸附的线 %d 条，形成 %d 个候选组" % (unsnapped, len(g)))

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

print("【步骤4】行线成组：%d 段" % len(segs))

W_LO, W_HI = 17000, 36000
Hb_LO, Hb_HI = 27000, 54000
cab = [s for s in segs
       if W_LO <= s["x1"] - s["x0"] <= W_HI and Hb_LO <= s["y1"] - s["y0"] <= Hb_HI and s["n"] >= 5]
cab.sort(key=lambda s: (-s["y1"], s["x0"]))
print("【步骤5】柜体量级：%d 台" % len(cab))
if cab:
    ws = sorted(s["x1"] - s["x0"] for s in cab)
    hs = sorted(s["y1"] - s["y0"] for s in cab)
    ns = sorted(s["n"] for s in cab)
    print("        宽 中位 %d [%d~%d]   高 中位 %d [%d~%d]   行数 中位 %d [%d~%d]"
          % (ws[len(ws) // 2], ws[0], ws[-1], hs[len(hs) // 2], hs[0], hs[-1],
             ns[len(ns) // 2], ns[0], ns[-1]))
    # 同一 y 带上的台数（按 y1 分排）
    bands = defaultdict(int)
    for s in cab:
        bands[round(s["y1"] / 20000) * 20000] += 1
    print("        按高度分排：", sorted(bands.items(), reverse=True))
    for s in cab[:12]:
        print("        %7d..%7d  y %7d..%7d  %dx%d  %d行"
              % (s["x0"], s["x1"], s["y0"], s["y1"], s["x1"] - s["x0"], s["y1"] - s["y0"], s["n"]))

json.dump(cab, open(r"G:\PyArx\viewer\export\cab_vision.json", "w", encoding="utf-8"),
          ensure_ascii=False, indent=1)
print("写出 export\\cab_vision.json")
