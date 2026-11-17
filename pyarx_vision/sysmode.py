# -*- utf-8 -*-
"""系统图（单线配电系统图）识别。

版式事实（实测 DRB018 #4）：
    - 一箱 = 一段连续的文字列，箱与箱之间 x 间隔 ≈16000，箱内列间隔 ≤3200；
    - 箱内回路号自成一列（WL1/W1/WC5…），每个回路号锚定一行；
    - 同一行按 x 排：[断路器][相序 L1/L2/L3][回路号][电缆][描述/功率]；
    - 描述与功率常分两行（描述在上、"0.8KW"在下），所以要按锚放宽 y；
    - 箱头有 型号(PZ30M) / 箱编号(101-1-1AL2) / 名称(洁净电箱)。

省略有三种，都必须认：
    1) 图形三点：同 x、等距 ≈1344、半径一致的 3~5 个 CIRCLE 一组；
    2) 文字区间：`WC4~WC10同上`，表示该段全部同前（可展开计数）；
    3) 箱底注：`注：图中未见回路为备用`，说明省画的回路按备用计。
"""

import re
import collections

RE_BREAKER = re.compile(r'(MCB|RCBO|MCCB|RCD|C65N|C65|NXB|DZ47|NA\d{2,}|NM\d{2,}|CM\d{2,}|\d{1,3}A\s*[/\s]\s*\d?P)', re.I)
RE_CABLE = re.compile(r'(BVR|BV|YJV|YJLV|WDZ|NH|JDG|SC\d|CT\b)', re.I)
RE_KW = re.compile(r'([\d.]+)\s*[Kk][Ww]')
RE_LOOPNO = re.compile(r'^(W[A-Z]{0,2}\d+|WL\d+|W\d+|\d{1,3})$')
RE_PHASE = re.compile(r'^L[123]$')
RE_SAME = re.compile(r'([A-Z]{0,3}\d{1,3})\s*[~～]\s*([A-Z]{0,3}\d{1,3})\s*(同上|同前)')
# 漏电保护参数（30mA 0.1s）不是回路描述，不能占 name
RE_RCD = re.compile(r'^\d{1,3}\s*mA|^\d\.\d{1,2}s$|^IΔn|^ΔI')
# "WP2 ~WP8 NA63-32A/3P UPS 3KW"：一条画出来、文字覆盖到 WP8
RE_RANGE = re.compile(r'[~～]\s*([A-Za-z]{0,4})(\d{1,3})')


def _cluster(vals, tol):
    cl = []
    for v in sorted(vals):
        if cl and v - cl[-1][1] <= tol:
            cl[-1][1] = v
        else:
            cl.append([v, v])
    return [(a, b) for a, b in cl]


def _cw(s):
    """粗略字宽（em）：全角/中文 1.0，ASCII 0.55。SHX 与 TTF 差异在这层不追。"""
    return sum(1.0 if ord(c) > 127 else 0.55 for c in s)


def enrich(t):
    """把插入点/附着点换算成文字包围盒。

    CAD 里 TEXT 的 x 是"对齐基准点"：ha=0 左端、ha=1 中心、ha=2 右端、ha=4 居中；
    y 是基线(va=0)/底(1)/中(2)/顶(3)。MTEXT 的 attachment 已在扫描端映射成同一套
    ha/va 值。识别端若直接拿 x/y 比大小，居中的 465 条文字就会整体右移半个字宽，
    y 也会偏半个字高 —— 柜列归属和回路 band 都会错。
    """
    s = (t["t"] or "").strip()
    h = float(t.get("h") or 250.0)
    wf = float(t.get("wf") or 1.0)
    w = _cw(s) * h * wf
    x = float(t["x"])
    y = float(t["y"])
    ha = int(t.get("ha") or 0)
    va = int(t.get("va") or 0)
    if ha == 1 or ha == 4:
        left = x - w / 2.0
    elif ha == 2:
        left = x - w
    else:
        left = x
    if va == 0:          # baseline
        cy = y + h * 0.35
    elif va == 1:        # bottom
        cy = y + h / 2.0
    elif va == 3:        # top
        cy = y - h / 2.0
    else:                # middle
        cy = y
    t2 = dict(t)
    t2["lx"] = left
    t2["rx"] = left + w
    t2["cx"] = left + w / 2.0
    t2["cy"] = cy
    t2["w"] = w
    return t2


def _texts(scan, frame):
    x0, y0, x1, y1 = frame
    return [enrich(t) for t in scan["texts"]
            if x0 <= t["x"] <= x1 and y0 <= t["y"] <= y1 and (t["t"] or "").strip()]


def _dots(scan, frame):
    """三点省略号：同 x、等距、同半径，≥3 个。返回 [(x, y_center, n)]。"""
    x0, y0, x1, y1 = frame
    cir = [c for c in scan["circles"] if x0 <= c[0] <= x1 and y0 <= c[1] <= y1 and c[2] > 0]
    if not cir:
        return []
    out = []
    for r in set(round(c[2]) for c in cir):
        pts = [c for c in cir if abs(round(c[2]) - r) <= 2]
        if len(pts) < 3:
            continue
        for lo, hi in _cluster([round(c[0]) for c in pts], tol=40):
            group = sorted([c for c in pts if lo <= c[0] <= hi], key=lambda c: c[1])
            if len(group) < 3:
                continue
            gaps = [group[i + 1][1] - group[i][1] for i in range(len(group) - 1)]
            med = sorted(gaps)[len(gaps) // 2]
            if med <= 0:
                continue
            cur = [group[0]]
            for i, g in enumerate(gaps):
                if abs(g - med) <= 0.25 * med:
                    cur.append(group[i + 1])
                else:
                    if len(cur) >= 3:
                        out.append(((lo + hi) / 2.0, (cur[0][1] + cur[-1][1]) / 2.0, len(cur)))
                    cur = [group[i + 1]]
            if len(cur) >= 3:
                out.append(((lo + hi) / 2.0, (cur[0][1] + cur[-1][1]) / 2.0, len(cur)))
    return out


def _boxes(ts, gap=5000):
    if not ts:
        return []
    groups = []
    for lo, hi in _cluster([t["lx"] for t in ts], tol=300):
        if groups and lo - groups[-1][1] <= gap:
            groups[-1][1] = max(groups[-1][1], hi)
        else:
            groups.append([lo, hi])
    return groups


def parse_system(scan, frame, frames=None):
    """frames 给定时按"柜体矩形框"逐台识别（一台柜一箱，坐标即框坐标）；
    否则退回按 x 聚类的老口径。"""
    ts = _texts(scan, frame)
    if not ts:
        return {"ok": False, "boxes": [], "dots": []}
    h_med = sorted(t["h"] for t in ts)[len(ts) // 2]
    dots = _dots(scan, frame)
    out = []

    regions = []
    if frames:
        xs_all = sorted(set(round(b["x0"]) for b in frames) | set(round(b["x1"]) for b in frames))
        for b in frames:
            # 柜框只是"柜体"的框，回路号/断路器/电缆/功率描述整列都在框右边，
            # 实测功率列在框右外 6.3 米（#12 FJAP1：框 x1=467840，功率 x=474158）。
            # 所以右界要扩到下一列边界为止，不能固定一个小余量。
            nxt = [x for x in xs_all if x > b["x1"] + 500]
            xr = min(b["x1"] + 7000, (nxt[0] - 300) if nxt else b["x1"] + 7000)
            regions.append((b["x0"], b["x1"], b.get("y0"), b.get("y1"), xr))
    else:
        for xa, xb in _boxes(ts):
            regions.append((xa, xb, None, None, None))

    for rg in regions:
        xa, xb, ya, yb = rg[0], rg[1], rg[2], rg[3]
        xr = rg[4] if len(rg) > 4 and rg[4] else xb + 1700
        mem = [t for t in ts if xa - 300 <= t["lx"] <= xr]
        if ya is not None and yb is not None:
            mem = [t for t in mem if ya - h_med * 1.5 <= t["y"] <= yb + h_med * 1.5]
        if len(mem) < 5:
            continue
        # 回路号列 = loopno 命中率最高的 x 列
        best = None
        for lo, hi in _cluster([t["lx"] for t in mem], tol=300):
            col = [t for t in mem if lo <= t["lx"] <= hi]
            hit = sum(1 for t in col if RE_LOOPNO.match(t["t"].strip()))
            if hit >= 3 and (best is None or hit > best[2]):
                best = (lo, hi, hit)
        if best is None:
            continue
        lx = (best[0] + best[1]) / 2.0
        # 一个箱内可能有两列回路号（普通回路 + 应急/备用回路并排），
        # 只取命中率最高那一列会漏掉另一列（实测 JXAL2 只取到 9/16）。
        # 所以按 y 去重收全：同一行若有多个回路号文字，保留最左一个作锚。
        cand = [t for t in mem
                if RE_LOOPNO.match(t["t"].strip()) and len(t["t"].strip()) <= 8
                and t["lx"] >= best[0] - 2500]
        anchors = []
        for t in sorted(cand, key=lambda t: (t["y"], t["lx"])):
            if anchors and abs(t["y"] - anchors[-1]["y"]) <= h_med * 0.9:
                continue
            anchors.append(t)
        if not anchors:
            continue
        # 锚之间的 y 间距
        gaps = [anchors[i + 1]["y"] - anchors[i]["y"] for i in range(len(anchors) - 1)]
        step = sorted(gaps)[len(gaps) // 2] if gaps else h_med * 2
        ytol = max(h_med * 1.3, min(step * 0.55, h_med * 2.5))

        # 一台柜一箱：回路由柜框（含框内 y 范围）归属，不再按 y 猜分段。
        # （试过按 y 大间隙切→拆出 100 个箱；按柜编号文字切→编号会作为引用
        #  出现在别处，拆出 423 条回路。只有柜框是可靠的一等公民。）
        segs = [anchors]

        for seg in segs:
            if len(seg) < 2:
                continue
            sg = [seg[i + 1]["y"] - seg[i]["y"] for i in range(len(seg) - 1)]
            sstep = sorted(sg)[len(sg) // 2] if sg else step
            sub = [t for t in mem
                   if seg[0]["y"] - sstep * 1.8 <= t["y"] <= seg[-1]["y"] + sstep * 1.0]
            loops = []
            same_expand = []
            note = ""
            # 区间归属：第 i 个回路号拿走 [y_i-0.5step, y_i+0.5step) 内的全部文字。
            # 描述与功率常掉在锚下方半行处，固定半径会漏，区间不会。
            spans = []
            for i, t in enumerate(seg):
                lo = t["y"] - sstep * 0.5
                hi = (t["y"] + sstep * 1.2 if i + 1 == len(seg)
                      else t["y"] + sstep * 0.5)
                spans.append((lo, hi))

            for i, t in enumerate(seg):
                no = t["t"].strip()
                lo, hi = spans[i]
                band = [q for q in sub if lo <= q["y"] < hi]
                brk = ""
                brkx = -1e9
                cab = ""
                cabx = -1e9
                desc = []
                for q in sorted(band, key=lambda q: q["lx"]):
                    s = q["t"].strip()
                    if RE_SAME.search(s):
                        m = RE_SAME.search(s)
                        same_expand.append({"from": m.group(1), "to": m.group(2),
                                            "text": s, "y": round(q["y"])})
                        continue
                    # 说明句：长文本或以"注"开头，归 note，不当回路字段
                    if "注：" in s or "注:" in s or len(s) > 26:
                        if not note:
                            note = s
                        continue
                    if RE_PHASE.match(s):
                        continue
                    if RE_RCD.search(s):
                        continue
                    if RE_LOOPNO.match(s) and q["lx"] >= lx - 250:
                        continue
                    # 柜编号与型号属于箱头，不是回路描述
                    # （实测 WL1 的 desc 会变成 "101-1-1AL3 GXL"）
                    if re.match(r'^\d{2,}[-—]\d', s):
                        continue
                    if re.match(r'^[A-Z]{2,}[- ]?\d{0,3}$', s) and len(s) <= 8:
                        continue
                    # 同一行常有两个器件文字：左边是进线总开关（NA125-63A/3P），
                    # 右边才是本回路的断路器。取最靠右的（贴近回路号列）才对。
                    if RE_BREAKER.search(s):
                        if not brk or q["rx"] > brkx:
                            brk, brkx = s, q["rx"]
                        continue
                    if RE_CABLE.search(s):
                        if not cab or q["rx"] > cabx:
                            cab, cabx = s, q["rx"]
                        continue
                    desc.append(s)
                kws = [float(m.group(1)) for s in desc for m in [RE_KW.search(s)] if m]
                loops.append({"no": no, "breaker": brk, "cable": cab,
                              "desc": " ".join([d for d in desc if not RE_KW.search(d)][:2])[:24],
                              "kw": max(kws) if kws else 0.0,
                              "spare": any("备用" in d for d in desc),
                              "y": round(t["y"])})

            # 图上常见"只画一条 + 文字写 WP2~WP8 同上"：一段文字代表多条回路，
            # 按区间展开成实体回路，否则整箱回路数会少一大截（实测 ITAP1 少 6 条）。
            exp = []
            for l in loops:
                dsc = (l["desc"] or "").strip()
                # 只有"整条描述就是 ~WP8 …"这种才是区间说明；
                # 描述中间出现 ~ 不能当展开依据，否则整箱回路会放大几十倍
                # （实测 FJAP1 从 18 涨到 169）。
                if not dsc.startswith(("~", "～")):
                    continue
                m = RE_RANGE.match(dsc)
                if not m:
                    continue
                m2 = re.search(r'([A-Za-z]{0,4})(\d{1,3})$', l["no"])
                if not m2:
                    continue
                cur2, tgt = int(m2.group(2)), int(m.group(2))
                if tgt <= cur2 + 1 or tgt - cur2 > 40:
                    continue
                pref = m2.group(1) or m.group(1)
                rest = (l["desc"][m.end():] or "").strip()[:40]
                for k in range(cur2 + 1, tgt + 1):
                    exp.append({**l, "no": "%s%d" % (pref, k), "desc": rest,
                                "y": round(l["y"] + (k - cur2) * sstep),
                                "expanded": True})
            if exp:
                loops = sorted(loops + exp, key=lambda l: l["y"])

            # 箱头：回路号列左侧、本段最靠上（y 最大）的那批；排除相序与回路号
            left = [t for t in sub if t["lx"] < lx - 300]
            ytop = max((t["y"] for t in left), default=seg[0]["y"])
            code = ""
            model = ""
            name = ""
            for t in sorted(left, key=lambda t: (-t["y"], t["lx"])):
                s = t["t"].strip()
                if t["y"] < ytop - h_med * 6:
                    break
                if RE_PHASE.match(s) or RE_LOOPNO.match(s):
                    continue
                if not code and re.match(r'^\d{2,}[-—]\d', s):
                    code = s
                elif not model and 2 <= len(s) <= 10 and re.match(r'^[A-Za-z]', s):
                    model = s
                elif not name and 2 <= len(s) <= 8 and re.search(r'[\u4e00-\u9fa5]', s):
                    name = s
            if not code:
                cand2 = [t["t"].strip() for t in left
                         if re.match(r'^\d{2,}[-—]\d', t["t"].strip())]
                code = cand2[0] if cand2 else ""

            omitted = sum(1 for dx, dy, n in dots
                          if xa - 300 <= dx <= xb + 300
                          and seg[0]["y"] - 400 <= dy <= seg[-1]["y"] + 400)
            out.append({
                "x0": round(xa), "x1": round(xb), "xr": round(xr), "lx": round(lx),
                "y0": round(ya if ya is not None else seg[0]["y"] - sstep * 1.8),
                "y1": round(yb if yb is not None else seg[-1]["y"] + sstep * 1.0),
                "code": code, "model": model, "name": name,
                "loops": loops, "drawn": len(loops),
                "omitted": omitted, "same_expand": same_expand,
                "note": note,
                "kw_sum": round(sum(l["kw"] for l in loops), 2),
                "spare": sum(1 for l in loops if l["spare"]),
            })
    return {"ok": bool(out), "boxes": out, "dots": dots}


def summary(res):
    boxes = res.get("boxes") or []
    drawn = sum(b["drawn"] for b in boxes)
    om = sum(b["omitted"] for b in boxes)
    same = sum(len(b["same_expand"]) for b in boxes)
    kw = round(sum(b["kw_sum"] for b in boxes), 2)
    spare = sum(b["spare"] for b in boxes)
    return {"boxes": len(boxes), "drawn": drawn, "omitted": om,
            "same": same, "kw": kw, "spare": spare}
