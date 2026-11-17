# -*- utf-8 -*-
"""pyarx_vision.geometry —— 几何原语：矩形判定、分桶、聚类、区间拆分

全部是无状态纯函数，便于单测。参数都用容差传入，不在源码里写死图纸尺度。
"""


def bucket(v, tol):
    """按 tol 容差归桶，用于同尺寸聚类（tol=50 时 1598/1603 落同桶）"""
    return int(round(v / tol)) * tol


def median(vs):
    vs = sorted(vs)
    n = len(vs)
    if n == 0:
        raise ValueError("median of empty")
    return vs[n // 2] if n % 2 else (vs[n // 2 - 1] + vs[n // 2]) / 2.0


def polygon_area(pts):
    """鞋带公式面积（绝对值）"""
    n = len(pts)
    s = 0.0
    for i in range(n):
        x1, y1 = pts[i]
        x2, y2 = pts[(i + 1) % n]
        s += x1 * y2 - x2 * y1
    return abs(s) / 2.0


def rect_of(pts, tol=1.0, area_tol=0.02):
    """顶点折线 → 轴对齐矩形 (x0,y0,x1,y1)；不是矩形返回 None

    判据两道，缺一不可：
      1. 所有顶点都落在包围盒的四条边上（允许 1 个闭合重复点）
      2. 多边形面积与包围盒面积相差不超过 area_tol
         —— 只有这一条能挡住"顶点在边上但中间凹进去"的伪矩形
    """
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
        if (abs(p[0] - x0) < tol or abs(p[0] - x1) < tol or
                abs(p[1] - y0) < tol or abs(p[1] - y1) < tol):
            on_edge += 1
    if on_edge < len(pts) - 1:
        return None
    a = polygon_area(pts)
    if abs(a - w * h) / (w * h) > area_tol:
        return None
    return (x0, y0, x1, y1)


def rect_center(r):
    return ((r[0] + r[2]) / 2.0, (r[1] + r[3]) / 2.0)


def clusters_by_value(items, value_fn, tol, min_size=1):
    """按某个标量把元素聚成簇（如按 y 把水平线分层）"""
    items = sorted(items, key=lambda x: -value_fn(x))
    out = []
    for it in items:
        v = value_fn(it)
        if out and abs(out[-1]["v"] - v) < tol:
            out[-1]["items"].append(it)
        else:
            out.append({"v": v, "items": [it]})
    return [c for c in out if len(c["items"]) >= min_size]


def split_by_gap(intervals, gap):
    """把若干 (x0,x1) 区间按"空档 > gap 就断开"合并成连续段"""
    if not intervals:
        return []
    ivs = sorted((min(a, b), max(a, b)) for a, b in intervals)
    out = [list(ivs[0])]
    for a, b in ivs[1:]:
        if a - out[-1][1] > gap:
            out.append([a, b])
        else:
            out[-1][1] = max(out[-1][1], b)
    return [(s[0], s[1]) for s in out]


def equidistant(xs, max_dev=0.15):
    """等距性检验：相邻间距与中位间距的相对偏差中位数不超过 max_dev"""
    if len(xs) < 3:
        return False, 0.0
    xs = sorted(xs)
    gaps = [xs[i + 1] - xs[i] for i in range(len(xs) - 1)]
    step = median(gaps)
    if step <= 0:
        return False, 0.0
    dev = median([abs(g - step) / step for g in gaps])
    return dev <= max_dev, step


def point_in_rect(p, r, pad=0.0):
    return (r[0] - pad <= p[0] <= r[2] + pad and
            r[1] - pad <= p[1] <= r[3] + pad)
