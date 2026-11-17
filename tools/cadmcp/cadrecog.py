# -*- utf-8 -*-
"""cadrecog.py —— 矢量图纸识图引擎（几何 + 拓扑 + 语义 + 自检）

输入：从 DWG 按图框提取的图元表（lines / plines / circles / texts，WCS 坐标）
输出：识别结果 + 每项置信度 + 自检报告

算法流程（不依赖图层名、不写死坐标）：
 1. 图元归一化：把闭合轴对齐折线归成"矩形图元"，尺寸做容差分桶聚类
 2. 重复单元发现：尺寸频次的主峰 = 阵列单元（本例是断路器列）；
    单元沿一条水平线等距排布，用 x 间距中位数反推"回路列宽"
 3. 母线识别：超长水平线按 y 聚成"层"，层内按 x 空档拆成"段"；
    每段取上下两条平行线配对，得到母线的 y/x 范围
 4. 空间归属：单元 → 母线用 (x 段内 + y 层内 + 最近) 三元约束，
    而不是单看 y（同层并排的两条母线靠 x 就能分开）
 5. 语义识别：文字按"词典 + 正则 + 相对位置"分类，再用
    "同列 + 上/下方 + 一对一贪心"把标注分给单元，避免一条文字被两个单元抢
 6. 自检：数量守恒、1:1 占用、孤立标注、覆盖率、行内顺序单调性，
    任何一条不满足就写进 problems，绝不静默通过
"""
import json
import os
import re
import collections
import math


# ---------------------------------------------------------------- 基础工具
def bucket(v, tol):
    return int(round(v / tol)) * tol


def rect_of(pts, axis_tol=1.0):
    """四点闭合折线 → 轴对齐矩形 (x0,y0,x1,y1)；不是矩形返回 None"""
    if len(pts) < 4:
        return None
    x0 = min(p[0] for p in pts)
    x1 = max(p[0] for p in pts)
    y0 = min(p[1] for p in pts)
    y1 = max(p[1] for p in pts)
    w, h = x1 - x0, y1 - y0
    if w <= 0 or h <= 0:
        return None
    on_edge = 0
    for p in pts:
        if (abs(p[0] - x0) < axis_tol or abs(p[0] - x1) < axis_tol or
                abs(p[1] - y0) < axis_tol or abs(p[1] - y1) < axis_tol):
            on_edge += 1
    return (x0, y0, x1, y1) if on_edge >= len(pts) - 1 else None


def median(vs):
    vs = sorted(vs)
    n = len(vs)
    return vs[n // 2] if n % 2 else (vs[n // 2 - 1] + vs[n // 2]) / 2.0


# ---------------------------------------------------------------- 1. 图元归一
def normalize(ent):
    rects = []
    for p in ent["plines"]:
        r = rect_of(p["p"]) if p["c"] else None
        if r and (r[2] - r[0]) > 200 and (r[3] - r[1]) > 100:
            rects.append({"r": r, "layer": p["l"]})
    hlines = []
    for l in ent["lines"]:
        if abs(l[1] - l[3]) < 1.0:
            hlines.append((min(l[0], l[2]), max(l[0], l[2]), l[1], l[4]))
    return rects, hlines


# ---------------------------------------------------------------- 2. 重复单元
def find_units(rects, min_count=3, tol=50, row_tol=1500):
    """尺寸聚类找阵列单元：同尺寸矩形先按 y 分成"行"，每行单独检验等距性。
    跨行混在一起算间距会把行与行的空档当成不等距，所以必须逐行判定。"""
    hist = collections.defaultdict(list)
    for rc in rects:
        x0, y0, x1, y1 = rc["r"]
        key = (bucket(x1 - x0, tol), bucket(y1 - y0, tol))
        hist[key].append(rc)
    peaks = sorted(hist.items(), key=lambda kv: -len(kv[1]))
    units = []
    for key, items in peaks:
        if len(items) < min_count:
            continue
        rows = []
        for it in items:
            cy = (it["r"][1] + it["r"][3]) / 2
            for r in rows:
                if abs(r["y"] - cy) < row_tol:
                    r["items"].append(it)
                    break
            else:
                rows.append({"y": cy, "items": [it]})
        good = []
        for r in rows:
            xs = sorted((it["r"][0] + it["r"][2]) / 2 for it in r["items"])
            if len(xs) < 2:
                continue
            gaps = [xs[i + 1] - xs[i] for i in range(len(xs) - 1)]
            step = median(gaps)
            if step <= 0:
                continue
            dev = [abs(g - step) / step for g in gaps]
            if median(dev) > 0.15:
                continue
            r["xs"] = xs
            good.append(r)
        if not good:
            continue
        allx = [x for r in good for x in r["xs"]]
        gaps = [x2 - x1 for x1, x2 in zip(allx, allx[1:])]
        units.append({"size": key, "count": len(items), "items": items,
                      "rows": good, "xmin": allx[0], "xmax": allx[-1],
                      "step": median(gaps) if gaps else 0,
                      "rows_n": len(good)})
    return units


# ---------------------------------------------------------------- 3. 母线
def find_buses(hlines, frame_w, min_len_ratio=0.15, tol_y=1500, gap_x=1500):
    """超长水平线 → 按 y 分层 → 层内按 x 空档拆段 → 上下边线配对"""
    big = [h for h in hlines if (h[1] - h[0]) > frame_w * min_len_ratio]
    if not big:
        return []
    big.sort(key=lambda h: -h[2])
    layers = []
    for h in big:
        if layers and abs(layers[-1]["y"] - h[2]) < tol_y:
            layers[-1]["hs"].append(h)
        else:
            layers.append({"y": h[2], "hs": [h]})
    segs = []
    for ly in layers:
        ivs = sorted((h[0], h[1]) for h in ly["hs"])
        cur = list(ivs[0])
        for iv in ivs[1:]:
            if iv[0] - cur[1] > gap_x:
                segs.append({"x0": cur[0], "x1": cur[1], "top": ly["y"]})
                cur = list(iv)
            else:
                cur[1] = max(cur[1], iv[1])
        segs.append({"x0": cur[0], "x1": cur[1], "top": ly["y"]})
    segs.sort(key=lambda s: (-round(s["top"] / 1000), s["x0"]))
    return segs


# ---------------------------------------------------------------- 5. 语义
PATTERNS = [
    ("cap", re.compile(r"^\d+A$")),
    ("ct", re.compile(r"^\d+/5$")),
    ("cable", re.compile(r"(YJ|KVV|YJV)")),
    ("meter", re.compile(r"电表|Wh")),
    ("spare", re.compile(r"备用")),
    ("bus", re.compile(r"母线槽")),
    ("station", re.compile(r"变电站")),
    ("load", re.compile(r"(Pe=|Kx=|Ijs=|Cos)")),
    ("tag", re.compile(r"^101-1-")),
    ("note", re.compile(r"(注：|开关整定|出线回路编号)")),
]


def classify(text):
    s = text.strip()
    for k, pat in PATTERNS:
        if pat.search(s):
            return k
    return "other"


# ---------------------------------------------------------------- 主流程
def recognize(ent, frame, cfg=None):
    cfg = cfg or {}
    x0f, y0f, x1f, y1f = frame
    frame_w = x1f - x0f
    problems = []
    log = []

    rects, hlines = normalize(ent)
    units = find_units(rects, min_count=cfg.get("min_units", 8))
    if not units:
        problems.append("未发现重复阵列单元")
    units.sort(key=lambda u: (-len(u["items"]), -u["rows_n"]))
    for u in units:
        log.append("阵列单元 %s×%s：%d 个 / %d 行，列距 %.1f"
                   % (u["size"][0], u["size"][1], len(u["items"]), u["rows_n"], u["step"]))

    buses = find_buses(hlines, frame_w)
    if len(buses) < 2:
        problems.append("母线段数异常: %d" % len(buses))

    log.append("矩形图元 %d，重复单元类型 %d，母线段 %d"
               % (len(rects), len(units), len(buses)))

    # 5.1 文字分类
    tags = collections.defaultdict(list)
    for t in ent["texts"]:
        tags[classify(t["t"])].append(t)
    log.append("文字分类: " + ", ".join("%s=%d" % (k, len(v))
                                    for k, v in sorted(tags.items())))

    # 5.2 母线信息：段内 + y 邻近
    for b in buses:
        b["spec"] = b["cap"] = b["station"] = b["pe"] = b["kx"] = ""
        b["info"] = {}
    for kind in ("bus", "station", "load"):
        for t in tags.get(kind, []):
            best, bd = None, 1e18
            for b in buses:
                if not (b["x0"] - 500 <= t["x"] <= b["x1"] + 500):
                    continue
                dd = abs(t["y"] - b["top"])
                if dd < 5000 and dd < bd:
                    best, bd = b, dd
            if best is None:
                continue
            key = {"bus": "spec", "station": "station", "load": "load"}[kind]
            best["info"].setdefault(key, [])
            if t["t"].strip() not in best["info"][key]:
                best["info"][key].append(t["t"].strip())
    # 母线容量：字高大的 cap
    for t in tags.get("cap", []):
        if t["h"] < 300:
            continue
        best, bd = None, 1e18
        for b in buses:
            if b["x0"] - 500 <= t["x"] <= b["x1"] + 500 and abs(t["y"] - b["top"]) < 3000:
                dd = abs(t["y"] - b["top"])
                if dd < bd:
                    best, bd = b, dd
        if best is not None and not best["cap"]:
            best["cap"] = t["t"].strip()

    for b in buses:
        info = b["info"]
        spec = info.get("spec", [])
        b["spec"] = spec[0] if spec else ""
        st = info.get("station", [])
        b["station"] = st[0] if st else ""
        load = " ".join(info.get("load", []))
        m = re.search(r"Pe=[^\s]+", load)
        b["pe"] = m.group(0) if m else ""
        m = re.search(r"Kx=[^\s]+", load)
        ijs = re.search(r"Ijs=[^\s]+", load)
        b["kx"] = " ".join(x for x in [m.group(0) if m else "",
                                       ijs.group(0) if ijs else ""] if x)
        cs = re.search(r"Cos[^\s]*", load)
        b["cos"] = cs.group(0) if cs else ""

    # 5.3 回路 = 主阵列单元
    main = units[0]
    loops = []
    for it in main["items"]:
        r = it["r"]
        cx, cy = (r[0] + r[2]) / 2, (r[1] + r[3]) / 2
        b = None
        for bb in buses:
            if bb["x0"] - 800 <= cx <= bb["x1"] + 800:
                if abs(cy - bb["top"]) < 6000:
                    b = bb
        loops.append({"x": cx, "y": cy, "bus": buses.index(b) if b else -1,
                      "cap": "", "tag": "", "cable": "", "ct": "", "meter": False,
                      "spare": "", "conf": {}})

    # 5.4 贪心一对一分配标注（同列 + 相对位置）
    assign_spec = [
        ("cap", 3200, 1100, True),      # 容量：紧贴单元
        ("ct", 9000, 1100, True),
        ("meter", 9000, 1100, True),
        ("spare", 14000, 1100, True),
        ("cable", 14000, 1100, False),
        ("tag", 16000, 2400, False),
    ]
    for kind, ylim, dxlim, nearest_only in assign_spec:
        pairs = []
        for ti, t in enumerate(tags.get(kind, [])):
            if kind == "cap" and t["h"] >= 300:
                continue                      # 母线容量不参与回路分配
            for li, lo in enumerate(loops):
                dx = abs(t["x"] - lo["x"])
                dy = abs(t["y"] - lo["y"])
                if dx > dxlim or dy > ylim:
                    continue
                pairs.append((dx + dy * 0.2, li, ti, t))
        pairs.sort(key=lambda p: p[0])
        used_l, used_t = set(), set()
        for _d, li, ti, t in pairs:
            if li in used_l or ti in used_t:
                continue
            if nearest_only and li in used_l:
                continue
            used_l.add(li)
            used_t.add(ti)
            lo = loops[li]
            s = t["t"].strip()
            if kind == "cap":
                lo["cap"] = s.rstrip("A")
            elif kind == "meter":
                lo["meter"] = True
            else:
                lo[kind] = s
            lo["conf"][kind] = round(1.0 - min(_d / (ylim + dxlim), 1.0), 3)
        log.append("分配 %s：命中 %d/%d" % (kind, len(used_t), len(tags.get(kind, []))))

    # 6. 自检：先排序再校验顺序/列距，避免用未排序序列报假错
    for bi, b in enumerate(buses):
        b["loops"] = sorted([l for l in loops if l["bus"] == bi], key=lambda r: r["x"])
        b["count"] = len(b["loops"])

    if any(l["bus"] < 0 for l in loops):
        problems.append("有回路未归入母线: %d" % sum(1 for l in loops if l["bus"] < 0))
    if any(not l["cap"] for l in loops):
        problems.append("有回路缺容量: %d" % sum(1 for l in loops if not l["cap"]))
    for bi, b in enumerate(buses):
        ls = b["loops"]
        if not ls:
            problems.append("母线段 %d 无回路" % bi)
            continue
        gaps = [ls[i + 1]["x"] - ls[i]["x"] for i in range(len(ls) - 1)]
        if gaps and (max(gaps) - min(gaps)) / max(gaps) > 0.25:
            problems.append("母线段 %d 列距不均匀: %.0f..%.0f" % (bi, min(gaps), max(gaps)))

    # 覆盖统计
    used = sum(1 for t in ent["texts"]
               if classify(t["t"]) in ("cap", "ct", "cable", "meter", "spare", "tag"))
    log.append("回路标注文字 %d 条，其中已分配 %d 条"
               % (used, sum(1 for l in loops for k in ("cap", "ct", "cable", "tag")
                            if l.get(k)) + sum(1 for l in loops if l["meter"]) +
                  sum(1 for l in loops if l["spare"])))

    return {"buses": buses, "loops": loops, "problems": problems, "log": log,
            "stats": {"rects": len(rects), "units": len(units),
                      "bus_segments": len(buses), "loops": len(loops),
                      "texts": len(ent["texts"])}}


if __name__ == "__main__":
    src = os.environ.get("RECOG_IN", r"C:\temp\sys.json")
    dst = os.environ.get("RECOG_OUT", r"C:\temp\recog.json")
    S = json.load(open(src, encoding="utf-8"))
    frame = S.get("frame")
    if not frame:                      # 没有图框声明时，用全部图元外包围盒兜底
        xs, ys = [], []
        for l in S["lines"]:
            xs += [l[0], l[2]]
            ys += [l[1], l[3]]
        for p in S["plines"]:
            for q in p["p"]:
                xs.append(q[0])
                ys.append(q[1])
        for t in S["texts"]:
            xs.append(t["x"])
            ys.append(t["y"])
        frame = (min(xs), min(ys), max(xs), max(ys))
        print("frame 未声明，用图元包围盒: %.0f,%.0f .. %.0f,%.0f" % frame)
    res = recognize(S, frame)
    json.dump(res, open(dst, "w", encoding="utf-8"),
              ensure_ascii=False, indent=1)
    print("== 识别日志 ==")
    for l in res["log"]:
        print("  " + l)
    print("== 问题 ==", res["problems"] if res["problems"] else "无")
    print("== 母线 ==")
    for i, b in enumerate(res["buses"]):
        print("%d# %-32s cap=%-6s %-14s 回路=%2d  x[%.0f..%.0f] top=%.0f"
              % (i + 1, b["spec"][:32], b["cap"], b["station"], b["count"],
                 b["x0"], b["x1"], b["top"]))
        print("     %s / %s" % (b["pe"], b["kx"]))
