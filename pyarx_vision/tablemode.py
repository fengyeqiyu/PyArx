# -*- utf-8 -*-
"""pyarx_vision.tablemode —— 表格法系统图识别

国内电力系统图常见的"表格法"版式：每个回路横向占一列，回路的属性
（计算容量 / 箱柜编号 / 进线电缆 / 整定电流 / 干线编号 / 所属变电所）
按行填写，标注竖排 90°。

识别顺序（与人的读图顺序一致）：
  1. 找表头行：含"整定电流/干线编号/计算容量…"的行标签，取该行文字作列锚
  2. 列距校验：相邻列 x 差的中位数，偏差过大视为离群列
  3. 逐列纵向聚合：把该列上下所有文字收进来（竖排文字按高度折算占位）
  4. 语义落格：用正则把聚合到的文字分到 容量/整定/编号/电缆/箱柜 各格
  5. 行标签反查：其余"xxx："标签给出每行的字段名，用于补充字段

全程不读图层名。
"""
import re

from . import frames as F

CAP_KW = re.compile(r"^\d+(\.\d+)?\s*(kW|KW)$")
RATED_A = re.compile(r"^\d+(\.\d+)?\s*A$")
CABLE = re.compile(r"(YJV|BV|WDZ|NH|KVVP|RVV|BVR|KVV)", re.I)
PANEL_NO = re.compile(r"^(?:\d+-)?\d+-(?:\d+-)?\w*?(ZAL|AL|AP|AT|UPS|AA|AM|KTAP|FQAP|RCUAP|FJAP|FFUAP|ITAP|HEFAP)", re.I)
TRUNK_NO = re.compile(r"(SUB|P\d+[/\~]|WP\d|WL\d)")
NAME_KW = re.compile(r"(配电[柜箱]|柜|箱)")
HEAD_WORDS = ("整定电流", "干线编号", "回路编号", "箱柜编号", "计算容量", "计容功率",
              "电缆", "开关型号", "出线编号", "回路名称", "备注")


def _label_rows(scan, frame):
    """行标签：靠近图框左缘、以冒号结尾的文字 → [(y, 字段名)]"""
    x0, y0, x1, y1 = frame
    W = x1 - x0
    out = []
    for t in scan.get("texts", []):
        if not (x0 <= t["x"] <= x1 and y0 <= t["y"] <= y1):
            continue
        s = t["t"].strip()
        if t["x"] < x0 + 0.12 * W and (s.endswith("：") or s.endswith(":")) and len(s) <= 12:
            out.append((round(t["y"], 1), s.rstrip("：:")))
    return out


def _anchor_row(scan, frame):
    """表头行：含 HEAD_WORDS 的行标签及其同行文字 → (y, 标签文字, 该行文字列表)

    两条排除条件（都是踩出来的）：
      · "注：xxx 出线编号" 这类说明文字也含表头词，但不是表头行
      · 真表头行一定是一排数据（同 y 至少有 5 条文字），说明行只有 1 条
    """
    x0, y0, x1, y1 = frame
    W = x1 - x0
    bad = ("注", "说明", "备注见", "详见", "见第")
    cand = []
    for t in scan.get("texts", []):
        if not (x0 <= t["x"] <= x1 and y0 <= t["y"] <= y1):
            continue
        s = t["t"].strip()
        if any(b in s for b in bad) and not s.endswith(("：", ":")):
            continue
        if any(w in s for w in HEAD_WORDS) and t["x"] < x0 + 0.14 * W:
            cand.append(t)
    if not cand:
        return None
    # 逐候选验：同行文字数
    best, best_n = None, -1
    for c in cand:
        row = [t for t in scan.get("texts", [])
               if abs(t["y"] - c["y"]) < max(150, c["h"] * 0.6)
               and x0 <= t["x"] <= x1 and t["t"].strip()]
        if len(row) >= 5 and len(row) > best_n:
            best, best_n = c, len(row)
    if best is None:
        return None
    y = best["y"]
    row = [t for t in scan.get("texts", [])
           if abs(t["y"] - y) < max(150, best["h"] * 0.6)
           and x0 <= t["x"] <= x1 and t["t"].strip()]
    return y, best["t"].strip().rstrip("：:"), row


def columns(scan, frame, max_dev=0.18):
    """列锚：表头行里除标签外的文字 → 校验等距后的列中心"""
    got = _anchor_row(scan, frame)
    if not got:
        return [], None
    y, lab_txt, row = got
    lab_x = min(t["x"] for t in row if any(w in t["t"] for w in HEAD_WORDS))
    # 列锚 = 表头行里位于行标签右侧的文字（不能用固定宽度比例，否则首列会被挡掉）
    cols = [t for t in row if t["x"] > lab_x + 1500]
    cols.sort(key=lambda t: t["x"])
    if len(cols) < 3:
        return [], (y, lab_txt, row)
    xs = [t["x"] for t in cols]
    diffs = [xs[i + 1] - xs[i] for i in range(len(xs) - 1)]
    diffs.sort()
    step = diffs[len(diffs) // 2]
    keep = [c for i, c in enumerate(cols)
            if i == 0 or abs((cols[i]["x"] - cols[i - 1]["x"]) - step) <= step * max_dev]
    return keep, (y, lab_txt, row)


def _vertical_span(t):
    """竖排文字的占位高度（锚点在起端，向上延伸）"""
    rot = abs(t.get("rotation", 0.0)) % 3.14159
    if 1.3 < rot < 1.85:
        return len(t["t"]) * t["height"] * 0.95
    return 0.0


def parse(scan, frame, dy_window=None):
    """表格法系统图 → {cols, step, rows(行标签), items(每个回路一格一行)}

    纵向窗口必须收住：以"行标签的 y 范围"为界，上下各留 1~3 个列距。
    放开就会把图框内的图例区、上方另一段表的文字吸进同一列（串列）。
    """
    cols, anchor = columns(scan, frame)
    if not cols:
        return {"ok": False, "reason": "未找到表头行（整定电流/干线编号…）", "items": []}
    x0, y0, x1, y1 = frame
    W = x1 - x0
    step = (cols[-1]["x"] - cols[0]["x"]) / max(len(cols) - 1, 1)
    labels = _label_rows(scan, frame)
    lab_x = min([t["x"] for t in anchor[2]
                 if any(w in t["t"] for w in HEAD_WORDS)] or [x0])
    if dy_window is None:
        ys = [y for y, _ in labels] or [anchor[0]]
        dy_lo = min(ys) - step * 1.2
        dy_hi = max(ys) + step * 3.2
    else:
        dy_lo, dy_hi = anchor[0] - dy_window, anchor[0] + dy_window * 2.2

    # 排除左缘行标签，其余都是数据文字
    texts = [t for t in scan.get("texts", [])
             if x0 <= t["x"] <= x1 and dy_lo <= t["y"] <= dy_hi and t["t"].strip()
             and t["x"] > lab_x + 1500]

    items = []
    for i, c in enumerate(cols):
        cx = c["x"]
        half = step * 0.45
        near = [t for t in texts if abs(t["x"] - cx) <= half]
        rec = {"no": i + 1, "x": round(cx, 1),
               "容量": "", "整定电流": "", "干线编号": "", "箱柜编号": "",
               "电缆": "", "备注": ""}
        used = set()
        for t in sorted(near, key=lambda t: -t["y"]):
            s = t["t"].strip()
            ink = "".join(s.split())
            key = (round(t["y"]), round(t["x"]))
            if key in used:
                continue
            if CAP_KW.match(ink) and not rec["容量"]:
                rec["容量"] = ink; used.add(key)
            elif RATED_A.match(ink) and not rec["整定电流"]:
                rec["整定电流"] = ink; used.add(key)
            elif CABLE.search(s) and not rec["电缆"]:
                rec["电缆"] = s[:70]; used.add(key)
            elif TRUNK_NO.search(s) and not NAME_KW.search(s) and not rec["干线编号"]:
                rec["干线编号"] = s; used.add(key)
            elif PANEL_NO.match(s) and not rec["箱柜编号"]:
                rec["箱柜编号"] = s; used.add(key)
            elif NAME_KW.search(s) and len(s) < 18 and not rec["备注"]:
                # "厂家自带配电柜"这类是备注，不能顶掉真正的箱柜编号
                rec["备注"] = s; used.add(key)
        # 行标签反查：语义没归上的，按最近行标签落到该字段
        for t in near:
            s = t["t"].strip()
            ink = "".join(s.split())
            if CAP_KW.match(ink) or RATED_A.match(ink) or CABLE.search(s) \
                    or TRUNK_NO.search(s) or PANEL_NO.match(s) or NAME_KW.search(s):
                continue
            best, bd = None, None
            for ly, nm in labels:
                d2 = abs(t["y"] - ly)
                if d2 < step * 3 and (bd is None or d2 < bd):
                    best, bd = nm, d2
            if best and not rec.get(best):
                rec[best] = s
        items.append(rec)

    filled = {k: sum(1 for r in items if r[k]) for k in ("整定电流", "容量", "干线编号", "箱柜编号", "电缆")}
    check = {"cols": len(cols), "step": round(step, 1), "filled": filled,
             "ok": filled["整定电流"] >= len(cols) - 1 and filled["干线编号"] >= len(cols) - 1}
    return {"ok": check["ok"], "check": check, "labels": [n for _, n in labels],
            "items": items}
