# -*- utf-8 -*-
"""geometry-first 提炼：以 DQ 断路器框(68 个)为回路主键，母线槽用横线端点定段。
校验要能回答：每条母线几回、每回有没有容量/编号/电缆/互感器/备用标注、
有没有"有标注没框"的孤立数据。
"""
import json
import re
import collections

S = json.load(open(r"C:\temp\sys.json", encoding="utf-8"))
TX, L, PL = S["texts"], S["lines"], S["plines"]


# ---- 1) 母线槽：长横线先按 y 聚成"段"（上下边线差 ~810 要合并），再按 x 间隙 >2000 拆 ----
hz = []
for l in L:
    if abs(l[1] - l[3]) < 1.0 and abs(l[0] - l[2]) > 20000:
        hz.append((min(l[0], l[2]), max(l[0], l[2]), l[1]))
hz.sort(key=lambda r: -r[2])
groups = []
for s in hz:
    if groups and abs(groups[-1]["y"] - s[2]) < 1500:
        groups[-1]["ys"].append(s[2])
        groups[-1]["segs"].append((s[0], s[1]))
    else:
        groups.append({"y": s[2], "ys": [s[2]], "segs": [(s[0], s[1])]})

buses = []
for g in groups:
    segs = sorted(g["segs"])
    cur = list(segs[0])
    for s in segs[1:]:
        if s[0] - cur[1] > 2000:
            buses.append({"x0": cur[0], "x1": cur[1], "y": max(g["ys"])})
            cur = list(s)
        else:
            cur[1] = max(cur[1], s[1])
    buses.append({"x0": cur[0], "x1": cur[1], "y": max(g["ys"])})
buses.sort(key=lambda r: (-round(r["y"] / 1000), r["x0"]))
print("母线槽段:", [(round(b["y"]), round(b["x0"]), round(b["x1"])) for b in buses])

# ---- 2) 回路 = DQ 框 ----
boxes = []
for p in PL:
    xs = [q[0] for q in p["p"]]
    ys = [q[1] for q in p["p"]]
    w, h = max(xs) - min(xs), max(ys) - min(ys)
    if p["l"] == "DQ" and 1400 < w < 1800 and 3000 < h < 3800:
        boxes.append({"x": sum(xs) / len(xs), "y": sum(ys) / len(ys),
                      "x0": min(xs), "x1": max(xs), "y0": min(ys), "y1": max(ys)})
print("DQ 框:", len(boxes))


def near(x, y, yw=1.0):
    best, bd = -1, 1e18
    for i, b in enumerate(buses):
        if not (b["x0"] - 800 <= x <= b["x1"] + 800):
            continue
        dd = abs(x - (b["x0"] + b["x1"]) / 2) * 0.2 + yw * abs(y - b["y"])
        if dd < bd:
            best, bd = i, dd
    return best


for bx in boxes:
    bx["bus"] = near(bx["x"], bx["y"], 1.0)
    # 段内剩余位置（从母线槽往上算序号，便于排序）
    bx["seq"] = 0

# ---- 3) 每列标签：同列 |dx|<1100，y 在框中心 ±15000 ----
def grab(bx, pred, ylim=15000, dxlim=1100):
    cand = []
    for t in TX:
        if not pred(t):
            continue
        if abs(t["x"] - bx["x"]) <= dxlim and abs(t["y"] - bx["y"]) <= ylim:
            cand.append((abs(t["y"] - bx["y"]), t))
    cand.sort(key=lambda r: r[0])
    return cand[0][1] if cand else None


is_cap = lambda t: re.fullmatch(r"\d+A", t["t"].strip())
is_ct = lambda t: re.fullmatch(r"\d+/5", t["t"].strip())
is_tag = lambda t: t["t"].strip().startswith("101-1-")
is_cable = lambda t: ("YJ" in t["t"] or "KVV" in t["t"])
is_meter = lambda t: "电表" in t["t"]
is_wh = lambda t: t["t"].strip() == "Wh"
is_spare = lambda t: "备用" in t["t"]

used_caps = set()
loops = []
for bx in boxes:
    lo = {"x": round(bx["x"], 1), "y": round(bx["y"], 1), "bus": bx["bus"],
          "cap": "", "tag": "", "cable": "", "ct": "", "meter": False,
          "spare": "", "note": ""}
    t = grab(bx, is_cap, 3000, 1100)
    if t:
        lo["cap"] = t["t"].strip().rstrip("A")
        used_caps.add(id(t))
    t = grab(bx, is_ct, 9000, 1100)
    if t:
        lo["ct"] = t["t"].strip()
    t = grab(bx, is_meter, 9000, 1100)
    if t:
        lo["meter"] = True
    t = grab(bx, is_spare, 14000, 1100)
    if t:
        lo["spare"] = t["t"].strip()
    loops.append(lo)

# 电缆 / 编号：一对一贪心配对（一条文字只能属于一个回路，避免同一标签被两个框抢走）
def pair(loops_, pred, dxlim, ylim, before):
    pairs = []
    for ti, t in enumerate(TX):
        if not pred(t):
            continue
        for li, lo in enumerate(loops_):
            if before and t["y"] <= lo["y"] - 200:      # 只在框上方找
                continue
            dx = abs(t["x"] - lo["x"])
            dy = abs(t["y"] - lo["y"])
            if dx <= dxlim and dy <= ylim:
                pairs.append((dx + dy * 0.15, li, ti, t))
    pairs.sort(key=lambda r: r[0])
    taken_l, taken_t = set(), set()
    for _d, li, ti, t in pairs:
        if li in taken_l or ti in taken_t:
            continue
        taken_l.add(li)
        taken_t.add(ti)
        lo = loops_[li]
        if pred == is_tag:
            lo["tag"] = t["t"].strip()
        else:
            lo["cable"] = t["t"].strip()


pair(loops, is_cable, 1100, 14000, True)
pair(loops, is_tag, 2600, 16000, True)

# ---- 4) 孤立容量标注（有标注但没框）----
orphan = []
for t in TX:
    if is_cap(t) and id(t) not in used_caps and t["h"] < 300:
        orphan.append((round(t["x"]), round(t["y"]), t["t"].strip()))
print("孤立容量标注(无框):", len(orphan), orphan[:8])

# ---- 5) 母线信息 ----
for i, b in enumerate(buses):
    bx_c = (b["x0"] + b["x1"]) / 2
    b["spec"] = b["cap"] = b["station"] = b["pe"] = b["kx"] = b["note"] = ""
    for t in TX:
        s = t["t"].strip()
        if not (b["x0"] - 500 <= t["x"] <= b["x1"] + 500 and abs(t["y"] - b["y"]) < 6000):
            continue
        if "母线槽" in s and not b["spec"]:
            b["spec"] = s
        if re.fullmatch(r"\d+A", s) and t["h"] >= 300 and not b["cap"]:
            b["cap"] = s
        if "变电站" in s and not b["station"]:
            b["station"] = s
        if "Pe=" in s and not b["pe"]:
            b["pe"] = s
        if "Kx" in s and not b["kx"]:
            b["kx"] = s

for i, b in enumerate(buses):
    ls = sorted([l for l in loops if l["bus"] == i], key=lambda r: r["x"])
    b["loops"] = ls
    b["count"] = len(ls)

out = {"title": "母线干线系统图（101-1-EL-SYS-21）",
       "source": "D:\\DeskTop\\汇总图-1002.dwg / A1 图框块 A$Cdee77b06",
       "unit": "A / kW",
       "buses": buses, "orphan_caps": orphan}
json.dump(out, open(r"C:\temp\trunk.json", "w", encoding="utf-8"),
          ensure_ascii=False, indent=1)

print()
print("%-32s %-6s %-14s %-3s %-24s %-24s" % ("母线", "容量", "变电站", "数", "Pe", "Kx"))
for i, b in enumerate(buses):
    print("%-32s %-6s %-14s %3d %-24s %-24s"
          % (b["spec"][:32], b["cap"], b["station"], b["count"],
             b["pe"][:24], b["kx"][:24]))
miss = collections.Counter()
for l in loops:
    if not l["cap"]: miss["无容量"] += 1
    if not l["tag"]: miss["无编号"] += 1
    if not l["cable"]: miss["无电缆"] += 1
    if not l["ct"]: miss["无CT"] += 1
    if not l["meter"]: miss["无电表"] += 1
    if l["spare"]: miss["备用"] += 1
print("回路 %d 条，缺项:" % len(loops), dict(miss))
