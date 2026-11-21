# -*- coding: utf-8 -*-
"""离线 DWG 解码 -> SVG 首版出图。遍历 ref_map 可解码实体，按几何类型收集 2D 图元。"""
import sys, math, json
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

REF = r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"
GEOM_NAMES = set(entity.TYPE_NAME.values())
GEOM_CODES = set(entity.TYPE_NAME.keys())


def _is_geom(p2):
    if p2 in GEOM_NAMES:
        return True
    try:
        return int(p2) in GEOM_CODES
    except ValueError:
        return False

# 类型名 -> 渲染分类
def collect():
    elems = {"line": [], "circle": [], "arc": [], "poly": [], "text": [],
             "point": [], "ellipse": [], "spline": [], "skip": 0, "err": 0}
    n = 0
    for line in open(REF, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) < 4:
            continue
        addr = int(p[1]); code = p[2]
        if not _is_geom(code):
            elems["skip"] += 1
            continue
        try:
            name, handle, common, g = entity.decode_entity(addr)
        except Exception:
            elems["err"] += 1
            continue
        if g is None:
            elems["skip"] += 1
            continue
        col = common.get("color", 256)
        try:
            if name == "LINE":
                elems["line"].append((g["sp"][0], g["sp"][1], g["ep"][0], g["ep"][1], col))
            elif name == "CIRCLE":
                elems["circle"].append((g["center"][0], g["center"][1], g["radius"], col))
            elif name == "ARC":
                elems["arc"].append((g["center"][0], g["center"][1], g["radius"],
                                     g["startAngle"], g["endAngle"], col))
            elif name == "LWPOLYLINE":
                pts = [(v["pt"][0], v["pt"][1]) for v in g["verts"]]
                elems["poly"].append((pts, g.get("closed", False), col))
            elif name == "TEXT":
                t = g["text"]
                t = t.decode("gbk", "replace").rstrip("\x00") if isinstance(t, bytes) else t
                elems["text"].append((g["pos"][0], g["pos"][1], g["height"],
                                      g["rotation"], t, col))
            elif name == "MTEXT":
                t = g["contents"]
                t = t.decode("gbk", "replace").rstrip("\x00") if isinstance(t, bytes) else t
                elems["text"].append((g["loc"][0], g["loc"][1], g["height"], 0.0, t, col))
            elif name == "POINT":
                elems["point"].append((g["pos"][0], g["pos"][1], col))
            elif name == "ELLIPSE":
                elems["ellipse"].append((g["center"][0], g["center"][1], g["majorAxis"],
                                         g["radiusRatio"], g["startAngle"], g["endAngle"], col))
            elif name == "SPLINE":
                pts = g["ctrl"] or g["fit"]
                elems["spline"].append(([(p[0], p[1]) for p in pts], col))
            else:
                elems["skip"] += 1
        except Exception:
            elems["err"] += 1
        n += 1
    return elems, n


def _bbox(el):
    xs, ys = [], []
    def add(x, y):
        xs.append(x); ys.append(y)
    for x1, y1, x2, y2, _ in el["line"]:
        add(x1, y1); add(x2, y2)
    for cx, cy, r, _ in el["circle"]:
        add(cx - r, cy - r); add(cx + r, cy + r)
    for cx, cy, r, sa, ea, _ in el["arc"]:
        add(cx - r, cy - r); add(cx + r, cy + r)
    for pts, _, _ in el["poly"]:
        for x, y in pts:
            add(x, y)
    for pts, _ in el["spline"]:
        for x, y in pts:
            add(x, y)
    for x, y, *_ in el["text"]:
        add(x, y)
    for x, y, _ in el["point"]:
        add(x, y)
    for cx, cy, *_ in el["ellipse"]:
        add(cx, cy)
    return min(xs), min(ys), max(xs), max(ys)


def _f(v):
    return f"{v:.3f}"


def _quantile(sorted_v, q):
    if not sorted_v:
        return 0.0
    return sorted_v[min(len(sorted_v) - 1, int(q * len(sorted_v)))]


def _in_box(x, y, box):
    xmin, ymin, xmax, ymax = box
    return xmin <= x <= xmax and ymin <= y <= ymax


def _clip_box(el, lo=0.005, hi=0.995):
    xs, ys = [], []
    def grab(pts):
        for x, y in pts:
            xs.append(x); ys.append(y)
    for e in el["line"]: grab([(e[0], e[1]), (e[2], e[3])])
    for e in el["circle"]: grab([(e[0], e[1])])
    for e in el["arc"]: grab([(e[0], e[1])])
    for e in el["poly"]: grab(e[0])
    for e in el["spline"]: grab(e[0])
    for e in el["text"]: grab([(e[0], e[1])])
    for e in el["point"]: grab([(e[0], e[1])])
    xs.sort(); ys.sort()
    box = (_quantile(xs, lo), _quantile(ys, lo), _quantile(xs, hi), _quantile(ys, hi))
    return box


def write_svg(el, out, pad=20.0):
    box = _clip_box(el)
    kept = {"line": [], "circle": [], "arc": [], "poly": [], "text": [],
            "point": [], "ellipse": [], "spline": []}
    for e in el["line"]:
        if _in_box(e[0], e[1], box) and _in_box(e[2], e[3], box): kept["line"].append(e)
    for e in el["circle"]:
        if _in_box(e[0], e[1], box): kept["circle"].append(e)
    for e in el["arc"]:
        if _in_box(e[0], e[1], box): kept["arc"].append(e)
    for pts, closed, c in el["poly"]:
        if all(_in_box(x, y, box) for x, y in pts): kept["poly"].append((pts, closed, c))
    for pts, c in el["spline"]:
        if all(_in_box(x, y, box) for x, y in pts): kept["spline"].append((pts, c))
    for e in el["text"]:
        if _in_box(e[0], e[1], box): kept["text"].append(e)
    for e in el["point"]:
        if _in_box(e[0], e[1], box): kept["point"].append(e)
    el = kept
    xmin, ymin, xmax, ymax = box
    w = xmax - xmin + 2 * pad; h = ymax - ymin + 2 * pad
    xmin -= pad; ymin -= pad; xmax += pad; ymax += pad
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{_f(xmin)} {_f(-ymax)} {_f(w)} {_f(h)}" '
             f'width="100%" height="100%" preserveAspectRatio="xMidYMid meet">',
             '<rect x="0" y="0" width="100%" height="100%" fill="#FFFFFF"/>',
             f'<g transform="scale(1,-1)" stroke="#111111" fill="none" vector-effect="non-scaling-stroke" stroke-width="1">']
    for x1, y1, x2, y2, _ in el["line"]:
        parts.append(f'<line x1="{_f(x1)}" y1="{_f(y1)}" x2="{_f(x2)}" y2="{_f(y2)}"/>')
    for cx, cy, r, _ in el["circle"]:
        parts.append(f'<circle cx="{_f(cx)}" cy="{_f(cy)}" r="{_f(r)}"/>')
    for cx, cy, r, sa, ea, _ in el["arc"]:
        x1 = cx + r * math.cos(sa); y1 = cy + r * math.sin(sa)
        x2 = cx + r * math.cos(ea); y2 = cy + r * math.sin(ea)
        large = 1 if (ea - sa) % (2 * math.pi) > math.pi else 0
        parts.append(f'<path d="M {_f(x1)} {_f(y1)} A {_f(r)} {_f(r)} 0 {large} 1 {_f(x2)} {_f(y2)}"/>')
    for pts, closed, _ in el["poly"]:
        d = "M " + " L ".join(f"{_f(x)} {_f(y)}" for x, y in pts)
        if closed:
            d += " Z"
        parts.append(f'<path d="{d}"/>')
    for pts, _ in el["spline"]:
        d = "M " + " L ".join(f"{_f(x)} {_f(y)}" for x, y in pts)
        parts.append(f'<path d="{d}"/>')
    for x, y, _ in el["point"]:
        parts.append(f'<circle cx="{_f(x)}" cy="{_f(y)}" r="1.5" fill="#111111"/>')
    parts.append("</g>")
    # 文字层不翻转：y 手动取负
    parts.append('<g fill="#111111" font-family="sans-serif" font-size="1">')
    for x, y, hgt, rot, t, _ in el["text"]:
        size = max(hgt, 1.0)
        parts.append(f'<text x="{_f(x)}" y="{_f(-y)}" font-size="{_f(size)}" '
                     f'text-anchor="start">{_esc(t)}</text>')
    parts.append("</g></svg>")
    open(out, "w", encoding="utf-8").write("\n".join(parts))
    return xmin, ymin, xmax, ymax


def _esc(t):
    return (t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
            .replace('"', "&quot;"))


# ---- 块参照展开（权威块内实体句柄来自 C 库 bh_ref.tsv）----
BH_REF = r"G:\PyArx\examples\cad2d\dwg1018\bh_ref.tsv"


def load_block_map():
    """返回 {块记录句柄: [实体句柄]} 与普通块内实体句柄集合（排除 Model/Paper Space）。"""
    block_ents, block_ent_set = {}, set()
    for line in open(BH_REF, encoding="gbk", errors="replace"):
        if "BLOCK_HEADER" not in line:
            continue
        p = line.rstrip("\n").split("\t")
        bh = int(p[0])
        if any(x.startswith("name=*") for x in p):
            continue
        ents = [int(x[1:]) for x in p if x[:1] == "E" and x[1:].isdigit()]
        block_ents[bh] = ents
        block_ent_set.update(ents)
    return block_ents, block_ent_set


def _classify(name, g, col):
    """实体几何 -> (渲染分类, 标准元组)；不可渲染返回 None。"""
    if name == "ATTDEF":
        return None
    if name == "LINE":
        return ("line", (g["sp"][0], g["sp"][1], g["ep"][0], g["ep"][1], col))
    if name == "CIRCLE":
        return ("circle", (g["center"][0], g["center"][1], g["radius"], col))
    if name == "ARC":
        return ("arc", (g["center"][0], g["center"][1], g["radius"],
                        g["startAngle"], g["endAngle"], col))
    if name == "LWPOLYLINE":
        pts = [(v["pt"][0], v["pt"][1]) for v in g["verts"]]
        return ("poly", (pts, g.get("closed", False), col))
    if name in ("TEXT", "ATTRIB"):
        t = g["text"]
        t = t.decode("gbk", "replace").rstrip("\x00") if isinstance(t, bytes) else t
        return ("text", (g["pos"][0], g["pos"][1], g["height"], g["rotation"], t, col))
    if name == "MTEXT":
        t = g["contents"]
        t = t.decode("gbk", "replace").rstrip("\x00") if isinstance(t, bytes) else t
        return ("text", (g["loc"][0], g["loc"][1], g["height"], 0.0, t, col))
    if name == "POINT":
        return ("point", (g["pos"][0], g["pos"][1], col))
    if name == "ELLIPSE":
        return ("ellipse", (g["center"][0], g["center"][1], g["majorAxis"],
                            g["radiusRatio"], g["startAngle"], g["endAngle"], col))
    if name == "SPLINE":
        pts = g["ctrl"] or g["fit"]
        return ("spline", ([(p[0], p[1]) for p in pts], col))
    return None


def _ins_tf(ins):
    px, py, _ = ins["pos"]
    rot = ins["rotation"]
    sx, sy, _ = ins["scale"]
    c, s = math.cos(rot), math.sin(rot)

    def tf(x, y):
        return (px + sx * x * c - sy * y * s,
                py + sx * x * s + sy * y * c)
    return tf, rot, sx, sy


def _xform(cat, tup, tf, drot, sx, sy):
    """把块内标准元组经 INSERT 变换到世界坐标。"""
    if cat == "line":
        x1, y1, x2, y2, c = tup
        a, b = tf(x1, y1); d, e = tf(x2, y2)
        return ("line", (a, b, d, e, c))
    if cat == "circle":
        cx, cy, r, c = tup
        nx, ny = tf(cx, cy)
        return ("circle", (nx, ny, r * (abs(sx) + abs(sy)) / 2 or r, c))
    if cat == "arc":
        cx, cy, r, sa, ea, c = tup
        nx, ny = tf(cx, cy)
        return ("arc", (nx, ny, r * (abs(sx) + abs(sy)) / 2 or r, sa + drot, ea + drot, c))
    if cat == "poly":
        pts, closed, c = tup
        return ("poly", ([tf(x, y) for x, y in pts], closed, c))
    if cat == "spline":
        pts, c = tup
        return ("spline", ([tf(x, y) for x, y in pts], c))
    if cat == "point":
        x, y, c = tup
        return ("point", (*tf(x, y), c))
    if cat == "text":
        x, y, hgt, rot, t, c = tup
        nx, ny = tf(x, y)
        return ("text", (nx, ny, hgt * abs(sy) or hgt, rot + drot, t, c))
    if cat == "ellipse":
        cx, cy, major, ratio, sa, ea, c = tup
        nx, ny = tf(cx, cy)
        mx, my, _ = major
        mc, ms = math.cos(drot), math.sin(drot)
        major = (mx * mc - my * ms, mx * ms + my * mc)
        return ("ellipse", (nx, ny, major, ratio, sa + drot, ea + drot, c))
    return (cat, tup)


def collect_expanded():
    """两遍解码：顶层实体直收；普通块内实体缓存；INSERT 按变换展开并入。"""
    elems = {"line": [], "circle": [], "arc": [], "poly": [], "text": [],
             "point": [], "ellipse": [], "spline": [], "skip": 0, "err": 0}
    block_ents, block_ent_set = load_block_map()
    geom_by_h = {}
    inserts = []
    for line in open(REF, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if len(p) < 4:
            continue
        addr = int(p[1]); code = p[2]
        if not _is_geom(code):
            elems["skip"] += 1
            continue
        try:
            name, handle, common, g = entity.decode_entity(addr)
        except Exception:
            elems["err"] += 1
            continue
        if g is None:
            elems["skip"] += 1
            continue
        col = common.get("color", 256)
        if name == "INSERT":
            inserts.append(g)
            continue
        if handle in block_ent_set:
            geom_by_h[handle] = (name, col, g)
            continue
        r = _classify(name, g, col)
        if r:
            elems[r[0]].append(r[1])
        else:
            elems["skip"] += 1

    for ins in inserts:
        ents = block_ents.get(ins["block"])
        if not ents:
            continue
        tf, drot, sx, sy = _ins_tf(ins)
        for eh in ents:
            cached = geom_by_h.get(eh)
            if not cached:
                continue
            name, col, g = cached
            r = _classify(name, g, col)
            if r:
                cat, tup = _xform(r[0], r[1], tf, drot, sx, sy)
                elems[cat].append(tup)
    return elems, len(inserts)


if __name__ == "__main__":
    el, n = collect()
    print("decoded", n)
    for k, v in el.items():
        print(k, len(v) if isinstance(v, list) else v)
    out = r"G:\PyArx\examples\cad2d\dwg1018\dwg_out.svg"
    box = write_svg(el, out)
    print("svg", out, "bbox", tuple(round(v, 1) for v in box))
