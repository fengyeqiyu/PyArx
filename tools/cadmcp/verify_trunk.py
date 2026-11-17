# -*- utf-8 -*-
"""交叉校验 trunk.json：用 DQ 断路器框(68 个 1600x3400) 反查回路位置，
再放宽匹配半径看漏配的编号/电缆是真缺失还是我关联偏了。"""
import json
import collections
import re

sysj = json.load(open(r"C:\temp\sys.json", encoding="utf-8"))
tr = json.load(open(r"C:\temp\trunk.json", encoding="utf-8"))
PL = sysj["plines"]
TX = sysj["texts"]

# 1) DQ 断路器框中心
boxes = []
for p in PL:
    xs = [q[0] for q in p["p"]]
    ys = [q[1] for q in p["p"]]
    w, h = max(xs) - min(xs), max(ys) - min(ys)
    if p["l"] == "DQ" and 1400 < w < 1800 and 3000 < h < 3800:
        boxes.append((sum(xs) / len(xs), sum(ys) / len(ys), w, h))
print("DQ 断路器框:", len(boxes))

loops = []
for bi, b in enumerate(tr["buses"]):
    for lo in b["loops"]:
        loops.append((bi, lo))
print("trunk 回路:", len(loops))

# 2) 每个框找最近的回路容量文字，看偏差
errs = []
for bx, by, w, h in boxes:
    best, bd = None, 1e18
    for bi, lo in loops:
        dd = abs(bx - lo["x"])
        if dd < bd:
            best, bd = lo, dd
    errs.append(bd)
errs.sort()
print("框→回路 x 偏差: min %.0f  p50 %.0f  p90 %.0f  max %.0f"
      % (errs[0], errs[len(errs) // 2], errs[int(len(errs) * 0.9)], errs[-1]))

# 3) 每段：框数 vs 回路数（框中心 y 与母线 y 的关系）
for bi, b in enumerate(tr["buses"]):
    nb = sum(1 for bx, by, w, h in boxes
             if abs(by - b["y"]) < 2600 and b["x0"] - 400 <= bx <= b["x1"] + 400)
    print("母线%d %-30s 框数=%2d 回路数=%2d %s"
          % (bi + 1, b["spec"][:30], nb, b["count"],
             "OK" if nb == b["count"] else "!! 不一致"))

# 4) 漏配编号：放宽到 ±2600 / y ±20000 能补回多少
tags = [t for t in TX if re.search(r"^101-1-", t["t"].strip())]
miss = [lo for bi, lo in loops if not lo["tag"]]
extra = 0
for lo in miss:
    hit = None
    for t in tags:
        if abs(t["x"] - lo["x"]) < 2600 and abs(t["y"] - lo["y"]) < 20000:
            hit = t["t"].strip()
    if hit:
        extra += 1
print("编号缺失 %d 个；放宽匹配半径后可补 %d 个" % (len(miss), extra))
print("原图 101-1- 编号总数:", len(tags))

# 5) 未分类文字抽样（检查有没有漏掉的回路/设备名）
def cls(t):
    s = t["t"].strip()
    for k, pat in (("cap", r"\d+A$"), ("cable", r"YJ"), ("ct", r"\d+/5$"),
                   ("meter", r"电表"), ("bus", r"母线槽"), ("station", r"变电站"),
                   ("pe", r"Pe="), ("kx", r"Kx"), ("tag", r"^101-1-")):
        if re.search(pat, s):
            return k
    return "other"

others = [t["t"].strip() for t in TX if cls(t) == "other"]
freq = collections.Counter(others)
print("未分类文字 %d 条，Top:" % len(others))
for s, c in freq.most_common(12):
    print("   %2d× %s" % (c, s[:44]))

# 6) 每段回路容量合计 vs 母线电流
for bi, b in enumerate(tr["buses"]):
    tot = sum(int(re.sub(r"[^0-9]", "", lo["cap"])) for lo in b["loops"])
    print("母线%d 回路容量合计 %dA　母线容量 %s　Ijs %s"
          % (bi + 1, tot, b.get("cap", ""), b.get("kx", "")))
