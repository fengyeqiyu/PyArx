"""找柜宽特征：水平长线的长度分布。柜体由一组等距水平行线构成，行线长度应约等于柜宽。"""
import json
from collections import Counter

SCENE = r"G:\PyArx\viewer\web\scene.json"
CAB = r"G:\PyArx\viewer\export\cabinets_all.json"
YMIN = 500000
BINW = 500

d = json.load(open(SCENE, encoding="utf-8"))
cabs = json.load(open(CAB, encoding="utf-8"))["cabinets"]

L = d["ln"]
hl = []
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if y1 < YMIN and y2 < YMIN:
        continue
    if abs(y2 - y1) > 10:
        continue
    ln = x2 - x1
    if 8000 <= ln <= 45000:
        hl.append((min(x1, x2), max(x1, x2), y1, ln))
print("水平长线（长 0.8~4.5 万）:", len(hl))

bins = Counter(int(ln // BINW) * BINW for _, _, _, ln in hl)
print("长度直方图 top10:")
for v, c in bins.most_common(10):
    print("   %d~%d : %d 条" % (v, v + BINW, c))

# 对每台柜，用框内的水平长线各自定一个"柜宽"，与当前框宽对比
print("\n每台柜：框内行线长度中位 vs 框宽（前 20 台）")
res = []
for c in cabs:
    b = c["box"]
    inl = [ln for x0, x1, y, ln in hl if b[0] <= x0 <= b[2] and b[1] <= y <= b[3]]
    bw = b[2] - b[0]
    med = sorted(inl)[len(inl) // 2] if inl else 0
    res.append({"i": c["i"] + 1, "name": c["name"], "boxw": bw, "rows": len(inl), "rowlen": med,
                "diff": bw - med if med else 0})
for r in res[:20]:
    print("  #%-2d %-16s 框宽 %-6d 行线 %-3d 长 %-6d 差 %d"
          % (r["i"], (r["name"] or "—")[:16], r["boxw"], r["rows"], r["rowlen"], r["diff"]))

has = [r for r in res if r["rows"] >= 6]
print("\n框内有 >=6 条水平长线的台数: %d / %d" % (len(has), len(res)))
if has:
    d = sorted(abs(r["diff"]) for r in has)
    print("框宽与行线长度之差: 中位 %d  四分位 %d~%d  吻合(差<1500) %d 台"
          % (d[len(d) // 2], d[len(d) // 4], d[3 * len(d) // 4], sum(1 for v in d if v < 1500)))
json.dump(res, open(r"G:\PyArx\viewer\export\cab_rows_len.json", "w", encoding="utf-8"),
          ensure_ascii=False, indent=1)
print("写出 export\\cab_rows_len.json")
