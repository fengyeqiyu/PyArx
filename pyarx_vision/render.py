# -*- utf-8 -*-
"""pyarx_vision.render —— 把识别出的每张图独立绘制成 PNG

只做"忠实还原"，不做美化、不按图层配色：图元统一深灰，文字黑。
关键是文字必须按 CAD 语义画：字高、旋转、宽度因子、对齐方式(ha/va)、字体样式。
"""
import math
import os

from PIL import Image, ImageDraw, ImageFont

FALLBACK_FONTS = [
    r"C:\Windows\Fonts\simhei.ttf",
    r"C:\Windows\Fonts\simsun.ttc",
    r"C:\Windows\Fonts\simfang.ttf",
]
_font_cache = {}


def _font(style, size):
    """按文字样式挑字体：TTF 样式用系统 TTF，SHX 样式用等宽近似"""
    st = (style or "").lower()
    key = (st, size)
    if key in _font_cache:
        return _font_cache[key]
    cand = []
    if "simhei" in st or "黑" in style or "":
        cand.append(r"C:\Windows\Fonts\simhei.ttf")
    if "simsun" in st or "song" in st:
        cand.append(r"C:\Windows\Fonts\simsun.ttc")
    if ".shx" in st:
        # SHX 是单线工程字体：用黑体近似（真矢量要走 tessellate，这里是出图核对）
        cand.append(r"C:\Windows\Fonts\simhei.ttf")
    cand.extend(FALLBACK_FONTS)
    f = None
    for p in cand:
        if os.path.exists(p):
            try:
                f = ImageFont.truetype(p, max(size, 6))
                break
            except Exception:
                continue
    if f is None:
        f = ImageFont.load_default()
    _font_cache[key] = f
    return f


def _text_img(txt, size, style, wf, rot, color=(20, 20, 20)):
    """文字画到透明层：先按宽度因子横向缩放，再旋转"""
    f = _font(style, size)
    w = int(size * (len(txt) + 2) * 1.25) + 12
    h = size * 2 + 10
    layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    ld = ImageDraw.Draw(layer)
    try:
        ld.text((6, h // 2), txt, font=f, fill=color + (255,), anchor="lm")
    except Exception:
        ld.text((6, h // 2 - size), txt, font=f, fill=color + (255,))
    if wf and 0.15 < wf < 3 and abs(wf - 1) > 0.06:
        layer = layer.resize((max(1, int(layer.width * wf)), layer.height), Image.LANCZOS)
    if rot:
        layer = layer.rotate(rot, expand=True, resample=Image.BICUBIC)
    return layer


def _paste_text(img, t, m, s, size_cap=120):
    size = int(round(t["h"] * s))
    # 个别文字字高读出来偏大（平面图里有 h=27000 这类），不封顶会把图面糊死
    size = min(max(size, 1), size_cap)
    if size < 4 or not t.get("t"):
        return
    txt = t["t"]
    rot = math.degrees(float(t.get("r", 0.0) or 0.0))
    wf = float(t.get("wf", 1.0) or 1.0)
    px, py = m(t["x"], t["y"])
    anchor = (int(t.get("ha", 0) or 0), int(t.get("va", 0) or 0))
    straight = abs(rot) < 0.6 and abs(wf - 1) <= 0.06
    if straight:
        f = _font(t.get("st"), size)
        va_map = {0: "s", 1: "d", 2: "m", 3: "a"}
        ha_map = {0: "l", 1: "m", 2: "r", 3: "m", 4: "m", 5: "l"}
        a = ha_map.get(anchor[0], "l") + va_map.get(anchor[1], "s")
        try:
            ImageDraw.Draw(img).text((px, py), txt, font=f, fill=(20, 20, 20), anchor=a)
            return
        except Exception:
            pass
    layer = _text_img(txt, size, t.get("st"), wf, rot)
    w, h = layer.size
    ha = anchor[0]
    va = anchor[1]
    ox = {"0": 6, "1": w // 2, "2": w - 6, "3": w // 2, "4": w // 2, "5": 6}.get(str(ha), 6)
    oy = {0: h - 6, 1: h - 6, 2: h // 2, 3: 6}.get(va, h - 6)
    img.paste(layer, (int(px - ox), int(py - oy)), layer)


def _draw_block_def(img, d, b, g, m, s):
    """把块定义几何按插入的 平移/旋转/缩放 变换后画到图面

    块定义里存的是块坐标系（相对基点）的坐标，插入点 p 是基点在世界的位置，
    所以 wx = px + sx·bx·cos(r) − sy·by·sin(r)。
    """
    px, py = b["p"]
    rot = float(b.get("r", 0.0) or 0.0)
    sx = float(b.get("s", 1.0) or 1.0)
    sy = float(b.get("sy", sx) or sx)
    c, sn = math.cos(rot), math.sin(rot)

    def xf(bx, by):
        return (px + sx * bx * c - sy * by * sn, py + sx * bx * sn + sy * by * c)

    for l in g.get("lines", []):
        d.line([m(*xf(l[0], l[1])), m(*xf(l[2], l[3]))], fill=(110, 110, 110), width=1)

    for p in g.get("plines", []):
        pts = p.get("p") or []
        if len(pts) < 2:
            continue
        xy = [m(*xf(q[0], q[1])) for q in pts]
        if p.get("c"):
            xy.append(xy[0])
        d.line(xy, fill=(110, 110, 110), width=1)

    for c0 in g.get("circles", []):
        p = m(*xf(c0[0], c0[1]))
        rr = max(c0[2] * s * abs(sx), 0.5)
        d.ellipse([p[0] - rr, p[1] - rr, p[0] + rr, p[1] + rr], outline=(110, 110, 110), width=1)

    for a in g.get("arcs", []):
        p = m(*xf(a[0], a[1]))
        rr = max(a[2] * s * abs(sx), 0.5)
        s0 = -(math.degrees(a[4]) + math.degrees(rot))
        s1 = -(math.degrees(a[3]) + math.degrees(rot))
        while s1 < s0:
            s1 += 360
        d.arc([p[0] - rr, p[1] - rr, p[0] + rr, p[1] + rr], s0, s1, fill=(110, 110, 110), width=1)

    for t in g.get("texts", []):
        wx, wy = xf(t["x"], t["y"])
        tt = {"x": wx, "y": wy, "t": t["t"], "h": t["h"] * abs(sx),
              "r": float(t.get("r", 0.0)) + rot, "wf": t.get("wf", 1.0),
              "ha": t.get("ha", 0), "va": 0, "st": ""}
        _paste_text(img, tt, m, s)


def render_sheet(scan, frame, path, target_w=1900, margin=34, grey=(105, 105, 105)):
    x0, y0, x1, y1 = frame
    W, H = max(x1 - x0, 1.0), max(y1 - y0, 1.0)
    s = (target_w - 2 * margin) / W
    hpx = int(H * s) + 2 * margin
    img = Image.new("RGB", (target_w, max(hpx, 120)), "white")
    d = ImageDraw.Draw(img)

    def m(x, y):
        return (margin + (x - x0) * s, margin + (y1 - y) * s)

    def inside(x, y):
        return x0 <= x <= x1 and y0 <= y <= y1

    # 折线（含闭合）
    for p in scan.get("plines", []):
        pts = p.get("p") or []
        if len(pts) < 2:
            continue
        xy = [m(q[0], q[1]) for q in pts]
        if p.get("c") and xy[0] != xy[-1]:
            xy.append(xy[0])
        if not any(inside(q[0], q[1]) for q in pts):
            # 可能跨越图框，只要有线段穿过就画
            if not (min(q[0] for q in pts) <= x1 and max(q[0] for q in pts) >= x0
                    and min(q[1] for q in pts) <= y1 and max(q[1] for q in pts) >= y0):
                continue
        d.line(xy, fill=grey, width=1)

    # 直线
    for l in scan.get("lines", []):
        a, b = (l[0], l[1]), (l[2], l[3])
        if max(a[0], b[0]) < x0 or min(a[0], b[0]) > x1:
            continue
        if max(a[1], b[1]) < y0 or min(a[1], b[1]) > y1:
            continue
        d.line([m(*a), m(*b)], fill=grey, width=1)

    # 圆
    for c in scan.get("circles", []):
        cx, cy, r = c[0], c[1], c[2]
        if not (x0 - r <= cx <= x1 + r and y0 - r <= cy <= y1 + r):
            continue
        p = m(cx, cy)
        rr = max(r * s, 0.4)
        d.ellipse([p[0] - rr, p[1] - rr, p[0] + rr, p[1] + rr], outline=grey, width=1)

    # 圆弧
    for a in scan.get("arcs", []):
        cx, cy, r = a["c"][0], a["c"][1], a["r"]
        if not (x0 - r <= cx <= x1 + r and y0 - r <= cy <= y1 + r):
            continue
        a0, a1 = math.degrees(a["a0"]), math.degrees(a["a1"])
        p = m(cx, cy)
        rr = max(r * s, 0.4)
        # CAD 逆时针、y 翻转 → PIL 的 start/end 取反
        s0, s1 = -a1, -a0
        while s1 < s0:
            s1 += 360
        d.arc([p[0] - rr, p[1] - rr, p[0] + rr, p[1] + rr], s0, s1, fill=grey, width=1)

    # 块参照：有块定义几何就按插入变换展开，没几何才退回 bbox 轮廓
    # 注意：本图大量块引用 getGeomExtents 抛 eNullExtents，b 为 null，
    # 不能靠 b 过滤，否则设备块全被跳过（平面图就只剩建筑轮廓）。
    defs = scan.get("block_defs") or {}
    for b in scan.get("blocks", []):
        nm = b.get("n")
        p0 = b.get("p")
        bb = b.get("b")
        g = ((defs.get(nm) or {}).get("geom") or {})
        if bb:
            if max(bb[0], bb[2]) < x0 or min(bb[0], bb[2]) > x1:
                continue
            if max(bb[1], bb[3]) < y0 or min(bb[1], bb[3]) > y1:
                continue
        elif p0:
            db = (defs.get(nm) or {}).get("b") or [0, 0, 0, 0]
            ext = max(abs(db[2] - db[0]), abs(db[3] - db[1])) * max(abs(float(b.get("s", 1) or 1)), 1.0)
            ext = max(ext, 300.0)
            if not (x0 - ext <= p0[0] <= x1 + ext and y0 - ext <= p0[1] <= y1 + ext):
                continue
        else:
            continue
        if g and p0:
            _draw_block_def(img, d, b, g, m, s)
            continue
        if bb:
            p1, p2 = m(bb[0], bb[1]), m(bb[2], bb[3])
            if abs(p2[0] - p1[0]) > 3:
                d.rectangle([min(p1[0], p2[0]), min(p1[1], p2[1]),
                             max(p1[0], p2[0]), max(p1[1], p2[1])], outline=(160, 160, 160))

    # 表格：网格 + 单元格文字
    for tb in scan.get("tables", []):
        bb = tb.get("b")
        if not bb:
            continue
        if max(bb[0], bb[2]) < x0 or min(bb[0], bb[2]) > x1:
            continue
        p1, p2 = m(bb[0], bb[1]), m(bb[2], bb[3])
        d.rectangle([min(p1[0], p2[0]), min(p1[1], p2[1]),
                     max(p1[0], p2[0]), max(p1[1], p2[1])], outline=(190, 190, 190))

    # 文字（最后画，压在图元上）
    cap = max(12, int(target_w * 0.035))
    for t in scan.get("texts", []):
        if not inside(t["x"], t["y"]):
            continue
        _paste_text(img, t, m, s, cap)

    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save(path, optimize=True)
    return path, img.size
