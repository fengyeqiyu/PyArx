# -*- utf-8 -*-
"""pyarx_vision.frames —— 认图：图框切分 / 图名 / 图种判定

只吃 scan.py 的扫描结果，不用任何图层名、不写死坐标。

三个判断都是几何 + 语义，且带交叉校验：
  find_frames   同尺寸闭合大矩形成族 → 图框；单张图也接受最大矩形
  sheet_title   图框内最大字号的语义文字（含"系统图/平面图/详图"）→ 图名
  classify      表格锚点 / 长横线 / 开关型号 / 导线型号 四路证据投票 → 图种
"""
import re

# 表格法系统图的表头词（一眼能认出"这是一张按行排属性的回路表"）
TABLE_HEADS = ("整定电流", "干线编号", "回路编号", "箱柜编号", "计算容量", "计容功率",
               "开关型号", "电缆规格", "出线编号", "回路名称")
# 图名里必带的词
TITLE_WORDS = ("系统图", "平面图", "详图", "剖面", "大样", "原理图")
SWITCH_RE = re.compile(r"(MCB|MCCB|RCBO|NA\d|C\d+A\s*[12]?P|\d+A/[123]P)", re.I)
WIRE_RE = re.compile(r"(YJV|BV|WDZ|NH|KVVP|RVV|BVR)", re.I)
CAP_RE = re.compile(r"^\d+(\.\d+)?\s*(kW|KW|kVA|W|A)$")


def _center(f):
    return ((f[0] + f[2]) / 2.0, (f[1] + f[3]) / 2.0)


def _frame_candidates(scan, min_w=12000, min_h=8000, max_w=600000, max_h=400000):
    """图框候选三路取证：

    1) 块参照 bbox —— 国内图集普遍把 A1/A0 图框做成块（本图 A\\$Cdee77b06 ×17）
    2) 闭合大矩形（frame_cands）—— 有的图直接画 pline 图框
    3) 线配对（两横两竖围成矩形）—— 少数图用 4 条 LINE
    三路合并后按尺寸成族，取"重复出现的主族 + 面积显著更大的独立图框"。
    """
    cands = []
    for b in scan.get("blocks", []):
        bb = b.get("b")
        if not bb:
            continue
        w, h = bb[2] - bb[0], bb[3] - bb[1]
        if min_w < w < max_w and min_h < h < max_h:
            cands.append([bb[0], bb[1], bb[2], bb[3]])
    for b in scan.get("frame_cands") or []:
        cands.append(list(b))
    # 线配对：同 x 范围的上下横线 + 同 y 范围的左右竖线
    hs, vs = [], []
    for l in scan.get("lines", []):
        if abs(l[1] - l[3]) < 1.0 and abs(l[0] - l[2]) > min_w:
            hs.append((min(l[0], l[2]), max(l[0], l[2]), l[1]))
        if abs(l[0] - l[2]) < 1.0 and abs(l[1] - l[3]) > min_h:
            vs.append((min(l[1], l[3]), max(l[1], l[3]), l[0]))
    for p in scan.get("plines", []):
        pts = p.get("p") or []
        for i in range(len(pts) - 1):
            a, b2 = pts[i], pts[i + 1]
            if abs(a[1] - b2[1]) < 1.0 and abs(a[0] - b2[0]) > min_w:
                hs.append((min(a[0], b2[0]), max(a[0], b2[0]), a[1]))
            if abs(a[0] - b2[0]) < 1.0 and abs(a[1] - b2[1]) > min_h:
                vs.append((min(a[1], b2[1]), max(a[1], b2[1]), a[0]))
    hs.sort(key=lambda t: -t[2])
    used = set()
    for i, (x0, x1, y1) in enumerate(hs):
        if i in used:
            continue
        for j in range(i + 1, len(hs)):
            if j in used:
                continue
            x0b, x1b, y2 = hs[j]
            if abs(x0b - x0) < 500 and abs(x1b - x1) < 500 and 8000 < y1 - y2 < max_h:
                # 左右竖线闭合校验
                nv = sum(1 for a, b3, x in vs
                         if abs(x - x0) < 500 and abs(a - y2) < 500 and abs(b3 - y1) < 500)
                if nv >= 1:
                    cands.append([x0, y2, x1, y1])
                    used.add(i)
                    used.add(j)
                    break
    return cands


def find_frames(scan, tol=1500, min_repeat=2):
    """图框列表：块/矩形/线配对三路取证 → 尺寸成族 → 主族 + 独立大图框"""
    cands = _frame_candidates(scan)
    if not cands:
        return []
    fams = []
    for b in cands:
        w, h = b[2] - b[0], b[3] - b[1]
        for fam in fams:
            if abs(fam["w"] - w) < tol and abs(fam["h"] - h) < tol:
                fam["items"].append(b)
                break
        else:
            fams.append({"w": w, "h": h, "items": [b]})
    fams.sort(key=lambda f: -len(f["items"]))
    keep = []
    main = fams[0] if len(fams[0]["items"]) >= min_repeat else None
    if main:
        keep.extend(main["items"])
    for fam in fams:
        if fam is main:
            continue
        area = fam["w"] * fam["h"]
        # 独立图框：单张、面积 ≥ 主族 1.6 倍（A0/加长图幅）
        if main and area < main["w"] * main["h"] * 1.6:
            continue
        keep.extend(fam["items"])
    # 去重（同一图框可能既在块里又在矩形里）
    uniq = []
    for b in sorted(keep, key=lambda b: -((b[2] - b[0]) * (b[3] - b[1]))):
        c = _center(b)
        if any(abs(c[0] - _center(u)[0]) < 2000 and abs(c[1] - _center(u)[1]) < 2000
               for u in uniq):
            continue
        uniq.append(b)
    # 网格排序：先按 y 分行（从上到下），行内按 x
    uniq.sort(key=lambda b: (-round(_center(b)[1] / 20000.0), _center(b)[0]))
    # 序号重排 + 行号
    out, row, last_y = [], 0, None
    for i, b in enumerate(uniq):
        cy = _center(b)[1]
        if last_y is None or abs(last_y - cy) > 20000:
            row += 1
            last_y = cy
        out.append({"no": i + 1, "row": row, "frame": b,
                    "w": round(b[2] - b[0], 1), "h": round(b[3] - b[1], 1)})
    return out


def inside(scan, frame):
    """取落在图框内的图元（统一成 pyarx_vision 的 EntitySet 便于复用识别器）"""
    x0, y0, x1, y1 = frame
    es = {"source": scan.get("file", ""), "frame": frame,
          "lines": [], "plines": [], "texts": [], "circles": []}
    for l in scan.get("lines", []):
        if x0 <= l[0] <= x1 and y0 <= l[1] <= y1:
            es["lines"].append({"a": (l[0], l[1]), "b": (l[2], l[3]), "layer": l[4]})
    for p in scan.get("plines", []):
        if not p["p"]:
            continue
        q = p["p"][0]
        if x0 <= q[0] <= x1 and y0 <= q[1] <= y1:
            es["plines"].append({"pts": [(a[0], a[1]) for a in p["p"]],
                                 "closed": p["c"], "layer": p["l"]})
    for t in scan.get("texts", []):
        if x0 <= t["x"] <= x1 and y0 <= t["y"] <= y1:
            es["texts"].append({"pos": (t["x"], t["y"]), "text": t["t"], "height": t["h"],
                                "rotation": t["r"], "layer": t["l"],
                                "ha": t["ha"], "va": t["va"]})
    for c in scan.get("circles", []):
        if x0 <= c[0] <= x1 and y0 <= c[1] <= y1:
            es["circles"].append({"center": (c[0], c[1]), "r": c[2], "layer": c[3]})
    return es


def sheet_title(scan, frame):
    """图名：优先图框标题栏（右下区）+ 含图名关键词 + 大字号

    说明性文字（"详见平面图""做法见xx"）不是图名，直接排除。
    """
    x0, y0, x1, y1 = frame
    ts = [t for t in scan.get("texts", []) if x0 <= t["x"] <= x1 and y0 <= t["y"] <= y1]
    if not ts:
        return ""
    # 封面页：整页是模板表 + 图纸目录，图名藏在目录条目里，不能当图名取
    if any(("图纸目录" in t["t"]) or ("图册目录" in t["t"]) for t in ts):
        return "封面·图纸目录"
    # 目录续页：整页散落大量 图号(101-1-XX-XX) + 图幅(A1/A0) 成对出现。
    # 只用图号会误判系统图（干线编号 101-1-SUB-P1 也是这个形状），必须叠加图幅密度。
    codes = sum(1 for t in ts if re.match(r"^10\d-\d-\w{2,4}-", (t["t"] or "").strip()))
    sizes = sum(1 for t in ts if (t["t"] or "").strip() in ("A0", "A1", "A2", "A3"))
    if codes >= 8 and sizes >= 8:
        return "封面·图纸目录（续）"
    BAD = ("详见", "参见", "见平面", "见系统", "做法见", "另见", "见图", "所示")
    good = [t for t in ts
            if any(w in t["t"] for w in TITLE_WORDS) and not any(b in t["t"] for b in BAD)]
    if not good:
        good = [t for t in ts if not any(b in t["t"] for b in BAD)]

    def score(t):
        # 标题栏在右下角：x 靠右、y 靠下
        right = 1 if t["x"] > x0 + 0.5 * (x1 - x0) else 0
        bottom = 1 if t["y"] < y0 + 0.30 * (y1 - y0) else 0
        return (right * 2 + bottom * 2) * 1000 + t["h"] * 2 + min(len(t["t"]), 40)

    good.sort(key=score, reverse=True)
    return good[0]["t"].strip()[:40]


def evidence(scan, frame):
    """图种四路证据"""
    x0, y0, x1, y1 = frame
    W = x1 - x0
    ts = [t for t in scan.get("texts", []) if x0 <= t["x"] <= x1 and y0 <= t["y"] <= y1]
    heads = sorted({h for h in TABLE_HEADS for t in ts if h in t["t"]})
    # 长横线（≥ 40% 图宽，水平）
    longs = 0
    for l in scan.get("lines", []):
        if x0 <= l[0] <= x1 and y0 <= l[1] <= y1 and abs(l[1] - l[3]) < 1.0 \
                and abs(l[0] - l[2]) > W * 0.4:
            longs += 1
    for p in scan.get("plines", []):
        if not p["p"] or not (x0 <= p["p"][0][0] <= x1):
            continue
        ys = [q[1] for q in p["p"]]
        xs = [q[0] for q in p["p"]]
        if len(p["p"]) == 2 and abs(ys[0] - ys[1]) < 1.0 and abs(xs[0] - xs[1]) > W * 0.4:
            longs += 1
    vertical = sum(1 for t in ts if abs(abs(t["r"]) - 1.5708) < 0.2 or abs(abs(t["r"]) - 4.7124) < 0.2)
    switches = sum(1 for t in ts if SWITCH_RE.search(t["t"]))
    wires = sum(1 for t in ts if WIRE_RE.search(t["t"]))
    caps = sum(1 for t in ts if CAP_RE.search(t["t"].strip()))
    blocks = sum(1 for b in scan.get("blocks", []) if x0 <= b["p"][0] <= x1 and y0 <= b["p"][1] <= y1)
    tables = sum(1 for t in scan.get("tables", []) if t["b"] and x0 <= t["b"][0] <= x1)
    return {"heads": heads, "long_h": longs, "vertical": vertical, "switches": switches,
            "wires": wires, "caps": caps, "blocks": blocks, "tables": tables, "texts": len(ts)}


def classify(scan, frame):
    """投票出图种：table / trunk / panel / plan / unknown

    判据分三层，都是几何 + 语义：
      1) 表头锚点（整定电流/干线编号…）—— 表格法的最硬证据，权重 8
      2) 等尺寸矩形阵列（find_units 命中）—— 母线干线图的本质特征，
         单母线图一定有成排的断路器框；箱内详图没有
      3) 开关型号 / 导线型号文字密度 —— 用来区分箱内详图与平面图
    """
    from . import detect as D
    ev = evidence(scan, frame)
    votes = {"table": 0, "trunk": 0, "panel": 0, "plan": 0}
    # 阵列：只在开关型号够多时才划算（否则每次调用都贵）
    units = []
    if ev["switches"] >= 15:
        try:
            es = inside(scan, frame)
            rects = D.normalize_rects(es)
            units = [u for u in D.find_units(rects, min_count=6)
                     if u["count"] >= 6 and u["rows_n"] >= 1]
        except Exception:
            units = []
    ev["units"] = len(units)
    ev["unit_max"] = max((u["count"] for u in units), default=0)

    if len(ev["heads"]) >= 2 and ev["vertical"] >= 8:
        votes["table"] += 8
    elif len(ev["heads"]) >= 2:
        votes["table"] += 6
    elif len(ev["heads"]) == 1 and ev["tables"] >= 3:
        votes["table"] += 4
    # 母线干线图：必须有阵列（≥6 个同尺寸框成排）
    if ev["unit_max"] >= 6:
        votes["trunk"] += 6 + (2 if ev["long_h"] >= 3 else 0)
        votes["panel"] -= 2
    # 箱内详图：开关密集、无阵列
    if ev["switches"] >= 20 and ev["unit_max"] == 0:
        votes["panel"] += 6
    if ev["switches"] >= 40 and ev["tables"] >= 3:
        votes["panel"] += 2
    # 平面图：导线型号密集、块成片、无阵列
    if ev["wires"] >= 10 and ev["blocks"] >= 20 and ev["unit_max"] == 0:
        votes["plan"] += 5
    if ev["texts"] == 0:
        return "unknown", ev
    kind = max(votes, key=lambda k: votes[k])
    if votes[kind] <= 0:
        return "unknown", ev
    return kind, ev
