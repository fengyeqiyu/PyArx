"""按同排相邻参数表的区间切柜，每台算紧致 bbox，并对宽高/柜名/参数做自检报警。"""
import json
import math
import re

SCENE = r"G:\PyArx\viewer\web\scene.json"
OUT = r"G:\PyArx\viewer\export\cabinets.json"
d = json.load(open(SCENE, encoding="utf-8"))
ly = d.get("ly", [])
CAB = re.compile(r"^\d{3}-\d-\d+[A-Za-z]")

tables = []
for i, tb in enumerate(d["tb"]):
    bb = tb["b"]
    grid = tb.get("t") or []
    pe = ijs = kx = cos = pjs = None
    for row in grid:
        cells = [str(c) if c is not None else "" for c in row]
        for j, c in enumerate(cells):
            cs = c.strip()
            if cs == "Pe=" and j + 1 < len(cells):
                pe = cells[j + 1].strip()
            elif cs == "Ijs=" and j + 1 < len(cells):
                ijs = cells[j + 1].strip()
            elif cs == "Kx=" and j + 1 < len(cells):
                kx = cells[j + 1].strip()
            elif cs.startswith("COS") and j + 1 < len(cells):
                cos = cells[j + 1].strip()
            elif cs == "Pjs=" and j + 1 < len(cells):
                pjs = cells[j + 1].strip()
    tables.append({"i": i, "b": bb, "pe": pe, "ijs": ijs, "kx": kx, "cos": cos, "pjs": pjs})

# 按 y 分排：同一排的柜 y 相近
tables.sort(key=lambda t: -((t["b"][1] + t["b"][3]) / 2))
rows, cur = [], []
for t in tables:
    yc = (t["b"][1] + t["b"][3]) / 2
    if cur and abs(yc - (cur[-1]["b"][1] + cur[-1]["b"][3]) / 2) > 12000:
        rows.append(cur)
        cur = []
    cur.append(t)
if cur:
    rows.append(cur)
for r in rows:
    r.sort(key=lambda t: t["b"][0])
print("排数:", len(rows), " 每排表数:", [len(r) for r in rows])

# 图元点缓存（只取有坐标的）
pts = []
L = d["ln"]
for i in range(0, len(L) - 5, 6):
    pts.append((L[i], L[i + 1]))
    pts.append((L[i + 2], L[i + 3]))
for p in d["pl"]:
    raw = p["p"]
    for i in range(0, len(raw) - 1, 2):
        pts.append((raw[i], raw[i + 1]))
C = d["cr"]
for i in range(0, len(C) - 4, 5):
    pts.append((C[i], C[i + 1]))
for t in d["tx"]:
    pts.append((t[0], t[1]))

cabs = []
for t in d["tx"]:
    s = (t[6] or "").strip()
    if CAB.match(s) and len(s) <= 22 and (ly[t[7]] if 0 <= t[7] < len(ly) else "") != "L_NOTE_C":
        cabs.append((s, t[0], t[1]))

cuts = []
for ri, r in enumerate(rows):
    for k, t in enumerate(r):
        left = t["b"][0] - 2000
        right = (r[k + 1]["b"][0] - 2000) if k + 1 < len(r) else t["b"][0] + 32000
        ytop = t["b"][3]
        ybot = t["b"][1]
        xs, ys = [], []
        for x, y in pts:
            if left <= x < right:
                xs.append(x)
                ys.append(y)
        if xs:
            left, right = min(left, min(xs)), max(right, max(xs))
            ybot, ytop = min(ybot, min(ys)), max(ytop, max(ys))
        w, h = right - left, ytop - ybot
        # 柜名：区间内、离表最近的
        best, bd = None, 1e18
        for s, cx, cy in cabs:
            if left - 3000 <= cx <= right + 3000 and ybot - 3000 <= cy <= ytop + 3000:
                dd = (cx - t["b"][0]) ** 2 + (cy - (t["b"][1] + t["b"][3]) / 2) ** 2
                if dd < bd:
                    bd, best = dd, s
        warn = []
        if not best:
            warn.append("无柜名")
        if w < 12000 or w > 34000:
            warn.append("宽异常%.0f" % w)
        if h < 24000 or h > 62000:
            warn.append("高异常%.0f" % h)
        if not t["pe"]:
            warn.append("无Pe")
        cuts.append({"row": ri, "cab": best, "pe": t["pe"], "ijs": t["ijs"], "kx": t["kx"],
                     "cos": t["cos"], "pjs": t["pjs"], "box": [round(left), round(ybot), round(right), round(ytop)],
                     "w": round(w), "h": round(h), "warn": warn})

print("\n切出:", len(cuts), " 报警:", sum(1 for c in cuts if c["warn"]))
for c in cuts:
    print("  r%d %-20s Pe=%-9s Ijs=%-9s %sx%s %s"
          % (c["row"], (c["cab"] or "?")[:20], c["pe"], c["ijs"], c["w"], c["h"],
             ",".join(c["warn"]) or "ok"))
json.dump({"file": d["f"], "cuts": cuts}, open(OUT, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("\n写出", OUT)
