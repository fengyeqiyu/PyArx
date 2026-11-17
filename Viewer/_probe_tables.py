"""以 ACAD_TABLE 为锚：筛出真正的参数表（含 Pe=/Ijs=），配最近柜名，看能否一台一表。"""
import json
import math
import re

SCENE = r"G:\PyArx\viewer\web\scene.json"
d = json.load(open(SCENE, encoding="utf-8"))
ly = d.get("ly", [])
CAB = re.compile(r"^\d{3}-\d-\d+[A-Za-z]+$")

cabs = []
for t in d["tx"]:
    s = (t[6] or "").strip()
    if CAB.match(s) and len(s) <= 22:
        cabs.append((s, t[0], t[1], ly[t[7]] if 0 <= t[7] < len(ly) else "?"))


def nearest_cab(x, y, skip_note=True):
    best, bd = None, 1e18
    for s, cx, cy, l in cabs:
        if skip_note and l == "L_NOTE_C":
            continue
        dd = (cx - x) ** 2 + (cy - y) ** 2
        if dd < bd:
            bd, best = dd, (s, l)
    return (best, round(math.sqrt(bd))) if best else (None, None)


rows = []
for i, tb in enumerate(d["tb"]):
    bb = tb["b"]
    grid = tb.get("t") or []
    pe = ijs = kx = None
    for row in grid:
        cells = [str(c) if c is not None else "" for c in row]
        for j, c in enumerate(cells):
            cs = c.strip()
            if cs in ("Pe=", "Pe") and j + 1 < len(cells):
                pe = cells[j + 1].strip()
            if cs in ("Ijs=", "Ijs") and j + 1 < len(cells):
                ijs = cells[j + 1].strip()
            if cs in ("Kx=", "Kx") and j + 1 < len(cells):
                kx = cells[j + 1].strip()
    cx, cy = (bb[0] + bb[2]) / 2, (bb[1] + bb[3]) / 2
    cab, dist = nearest_cab(cx, cy)
    rows.append({"i": i, "b": bb, "w": bb[2] - bb[0], "h": bb[3] - bb[1],
                 "pe": pe, "ijs": ijs, "kx": kx, "cab": cab, "dist": dist,
                 "r": tb["r"], "c": tb["c"]})

para = [r for r in rows if r["pe"] or r["ijs"]]
print("表总数:", len(rows), " 参数表:", len(para), " 非参数表:", len(rows) - len(para))
print("表宽高分布:", sorted({(round(r["w"]), round(r["h"])) for r in rows}))
print("\n参数表清单（按 y 降序）：")
para.sort(key=lambda r: (-r["b"][1], r["b"][0]))
for r in para:
    print("  %-20s Pe=%-9s Ijs=%-9s Kx=%-5s 距%d  bbox=%s"
          % ((r["cab"] or "?")[:20], r["pe"], r["ijs"], r["kx"], r["dist"] or -1,
             [round(v) for v in r["b"]]))

names = [r["cab"] for r in para if r["cab"]]
print("\n有柜名的参数表:", len(names), " 唯一柜名:", len(set(names)))
print("同名多次:", sorted({n for n in names if names.count(n) > 1})[:10])
