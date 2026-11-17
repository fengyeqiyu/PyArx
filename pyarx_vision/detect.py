# -*- utf-8 -*-
"""pyarx_vision.detect —— 结构发现：阵列单元 + 母线槽段

两个发现器都只看几何，不依赖图层名，也不写死坐标：
  find_units  同尺寸矩形按频次找主峰，再逐行验等距 → 阵列单元（如断路器列）
  find_buses  超长水平线按 y 分层，层内按 x 空档拆段 → 母线段
"""
from . import geometry as G


def normalize_rects(es, min_w=200.0, min_h=100.0, tol=1.0):
    """闭合折线 → 矩形图元；尺寸过小的（符号、箭头）丢弃"""
    out = []
    for p in es["plines"]:
        if not p["closed"]:
            continue
        r = G.rect_of(p["pts"], tol)
        if r and (r[2] - r[0]) >= min_w and (r[3] - r[1]) >= min_h:
            out.append({"r": r, "layer": p["layer"]})
    return out


def find_units(rects, min_count=8, size_tol=50.0, row_tol=1500.0, max_dev=0.15):
    """同尺寸矩形 → 按 y 分行 → 逐行验等距 → 返回阵列单元

    返回按"个数优先、行数其次"排序，units[0] 即图面主阵列。
    """
    groups = {}
    for rc in rects:
        r = rc["r"]
        key = (G.bucket(r[2] - r[0], size_tol), G.bucket(r[3] - r[1], size_tol))
        groups.setdefault(key, []).append(rc)

    units = []
    for key, items in sorted(groups.items(), key=lambda kv: -len(kv[1])):
        if len(items) < min_count:
            continue
        rows = []
        for it in items:
            cy = G.rect_center(it["r"])[1]
            for row in rows:
                if abs(row["y"] - cy) < row_tol:
                    row["items"].append(it)
                    break
            else:
                rows.append({"y": cy, "items": [it]})
        good = []
        for row in rows:
            xs = [G.rect_center(it["r"])[0] for it in row["items"]]
            ok, step = G.equidistant(xs, max_dev)
            if ok:
                row["xs"] = sorted(xs)
                row["step"] = step
                good.append(row)
        if not good:
            continue
        units.append({"size": key, "count": len(items), "items": items,
                      "rows": good, "rows_n": len(good),
                      "step": G.median([r["step"] for r in good])})
    units.sort(key=lambda u: (-u["count"], -u["rows_n"]))
    return units


def find_buses(es, frame_w, min_len_ratio=0.15, y_tol=1500.0, gap=1500.0,
               edge_merge=2000.0):
    """超长水平线 → 按 y 分层 → 合并上下边线 → 按 x 空档拆段

    min_len_ratio：线长阈值占图宽比例，避免把尺寸线当成母线
    edge_merge：同段上下两条边线的 y 差上限（母线槽宽度）
    """
    hz = []
    for l in es["lines"]:
        a, b = l["a"], l["b"]
        if abs(a[1] - b[1]) < 1.0 and abs(a[0] - b[0]) > frame_w * min_len_ratio:
            hz.append((min(a[0], b[0]), max(a[0], b[0]), a[1]))
    if not hz:
        return []

    layers = G.clusters_by_value(hz, lambda h: h[2], y_tol)
    buses = []
    for ly in layers:
        segs = G.split_by_gap([(h[0], h[1]) for h in ly["items"]], gap)
        for s in segs:
            buses.append({"x0": s[0], "x1": s[1], "top": max(h[2] for h in ly["items"]),
                          "thick": max(h[2] for h in ly["items"]) -
                                   min(h[2] for h in ly["items"])})

    # 同 x 范围、y 差在 edge_merge 内、厚度接近的段合并为一条母线（吃掉上下边线）
    merged = []
    for b in sorted(buses, key=lambda r: (-r["top"], r["x0"])):
        for m in merged:
            if (abs(m["x0"] - b["x0"]) < 500 and abs(m["x1"] - b["x1"]) < 500 and
                    abs(m["top"] - b["top"]) < edge_merge):
                m["top"] = max(m["top"], b["top"])
                m["bottom"] = min(m.get("bottom", m["top"]), b["top"])
                break
        else:
            b["bottom"] = b["top"]
            merged.append(b)
    for m in merged:
        m["thick"] = m["top"] - m["bottom"]
    merged.sort(key=lambda r: (-round(r["top"] / 2000.0), r["x0"]))
    return merged


def group_rows(buses, y_tol=1200.0):
    """同一水平高度的母线并排成一行（图上 4# 与 2# 同排）"""
    rows = []
    for b in sorted(buses, key=lambda r: (-r["top"], r["x0"])):
        for r in rows:
            if abs(r["top"] - b["top"]) < y_tol:
                r["buses"].append(b)
                break
        else:
            rows.append({"top": b["top"], "buses": [b]})
    for r in rows:
        r["buses"].sort(key=lambda b: b["x0"])
    return rows


def assign_bus(unit_center, buses, pad=800.0, max_dy=6000.0):
    """单元归母线：x 落段内 + y 在层内 + 取最近（同排两条母线靠 x 就能分开）"""
    cx, cy = unit_center
    best, bd = None, None
    for i, b in enumerate(buses):
        if not (b["x0"] - pad <= cx <= b["x1"] + pad):
            continue
        dy = abs(cy - b["top"])
        if dy > max_dy:
            continue
        if bd is None or dy < bd:
            best, bd = i, dy
    return best if best is not None else -1
