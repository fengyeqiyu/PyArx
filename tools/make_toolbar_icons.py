# -*- coding: utf-8 -*-
"""Generate the vertical CAD-style drawing toolbar icons (transparent PNG).

Style follows the AutoCAD classic tool palette: thin light strokes on dark,
blue accent dots/handles. Output: G:\\PyArx\\plugins\\icons\\*.png (32x32).
"""
import os
from PIL import Image, ImageDraw

OUT = r"G:\PyArx\plugins\icons"
SS = 4          # supersample factor
S = 32          # logical size
CW, CH = 36, 32 # canvas (button width x height)
N = S * SS

LINE = (208, 217, 228, 255)
ACC = (62, 156, 245, 255)
GREEN = (60, 220, 130, 255)
DIM = (150, 163, 178, 255)


K = 1.38


def P(x, y):
    return ((19 + (x - 16) * K) * SS, (16 + (y - 16) * K) * SS)


def box(d, x0, y0, x1, y1, col=None, w=1.5):
    col = LINE if col is None else col
    d.line([P(x0, y0), P(x1, y0), P(x1, y1), P(x0, y1), P(x0, y0)], fill=col, width=int(w * SS))


def dot(d, x, y, r=1.4, col=None):
    col = ACC if col is None else col
    d.ellipse([P(x - r, y - r), P(x + r, y + r)], fill=col)


def square(d, x, y, r=1.6, col=None):
    col = ACC if col is None else col
    d.rectangle([P(x - r, y - r), P(x + r, y + r)], fill=col)


def poly(d, pts, col=None, w=1.4):
    col = LINE if col is None else col
    d.line([P(*p) for p in pts], fill=col, width=int(w * SS))


def arrow(d, x0, y0, x1, y1, col=None, w=1.0, head=2.6):
    col = ACC if col is None else col
    import math
    poly(d, [(x0, y0), (x1, y1)], col, w)
    ang = math.atan2(y1 - y0, x1 - x0)
    for da in (2.6, -2.6):
        poly(d, [(x1, y1),
                 (x1 - head * math.cos(ang - da / 2.0 * 1.1),
                  y1 - head * math.sin(ang - da / 2.0 * 1.1))], col, w)


def icon_osnap(d):
    for x in (8, 16, 24):
        for y in (8, 16, 24):
            d.rectangle([P(x - 0.7, y - 0.7), P(x + 0.7, y + 0.7)], fill=DIM)
    dot(d, 8, 8, 2.0)


def icon_line(d):
    poly(d, [(6, 26), (26, 6)], w=1.2)
    square(d, 6, 26, 2.2)
    square(d, 26, 6, 2.2)


def icon_pline(d):
    poly(d, [(5, 27), (14, 10), (22, 7)], w=1.2)
    square(d, 5, 27, 1.9)
    square(d, 14, 10, 1.9)
    square(d, 22, 7, 1.9)


def icon_arc(d):
    d.arc([P(4, 6), P(28, 30)], 200, 340, fill=LINE, width=int(1.7 * SS))
    square(d, 5, 24, 1.8)
    square(d, 27, 24, 1.8)


def icon_polygon(d):
    import math
    pts = []
    for i in range(5):
        a = -math.pi / 2 + i * 2 * math.pi / 5
        pts.append((16 + 11 * math.cos(a), 17 + 11 * math.sin(a)))
    pts.append(pts[0])
    poly(d, pts, w=1.2)
    square(d, 16, 6, 1.9)


def icon_rect(d):
    box(d, 5, 10, 27, 22, w=1.2)
    square(d, 5, 10, 1.9)
    square(d, 27, 22, 1.9)


def icon_arc2(d):
    d.arc([P(5, 8), P(27, 28)], 195, 350, fill=LINE, width=int(1.7 * SS))
    dot(d, 16, 12, 1.6)


def icon_circle(d):
    d.ellipse([P(6, 6), P(26, 26)], outline=LINE, width=int(1.7 * SS))
    dot(d, 16, 16, 1.4)


def icon_cloud(d):
    w = 1.7
    d.arc([P(3, 12), P(14, 24)], 175, 355, fill=LINE, width=int(w * SS))
    d.arc([P(9, 6), P(22, 24)], 170, 370, fill=LINE, width=int(w * SS))
    d.arc([P(17, 12), P(28, 24)], 175, 355, fill=LINE, width=int(w * SS))
    poly(d, [(4, 24), (27, 24)], w=1.4)


def icon_spline(d):
    d.arc([P(4, 18), P(16, 30)], 180, 360, fill=LINE, width=int(1.6 * SS))
    d.arc([P(10, 12), P(22, 24)], 0, 180, fill=LINE, width=int(1.6 * SS))
    d.arc([P(16, 6), P(28, 18)], 180, 360, fill=LINE, width=int(1.6 * SS))
    dot(d, 8, 13, 1.5)


def icon_ellipse(d):
    d.ellipse([P(4, 10), P(28, 22)], outline=LINE, width=int(1.7 * SS))
    square(d, 25, 20, 1.8)


def icon_ring(d):
    d.ellipse([P(4, 8), P(28, 24)], outline=LINE, width=int(1.7 * SS))
    d.ellipse([P(11, 13), P(21, 19)], outline=LINE, width=int(1.4 * SS))


def icon_copy(d):
    box(d, 5, 8, 18, 20)
    box(d, 12, 13, 25, 25, col=ACC)
    square(d, 12, 13, 1.6)


def icon_move(d):
    box(d, 5, 8, 15, 18, col=DIM)
    arrow(d, 16, 22, 27, 11, col=ACC, w=1.2)


def icon_point(d):
    for x, y in ((16, 9), (9, 16), (23, 16), (16, 24)):
        square(d, x, y, 1.5, ACC)


def icon_hatch(d):
    box(d, 5, 8, 27, 24, w=1.1)
    for i in range(4):
        poly(d, [(7 + i * 5, 22), (12 + i * 5, 10)], col=DIM, w=0.9)


def icon_image(d):
    box(d, 3, 9, 29, 25, w=1.5)
    poly(d, [(5, 23), (11, 15), (17, 22), (23, 16)], col=DIM, w=1.1)
    # star accent
    import math
    cx, cy, r = 24.0, 15.0, 3.6
    pts = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5
        rr = r if i % 2 == 0 else r * 0.45
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    poly(d, pts + [pts[0]], col=ACC, w=1.0)


def icon_block(d):
    poly(d, [(5, 7), (22, 7), (26, 11), (26, 25), (5, 25), (5, 7)], w=1.1)
    d.ellipse([P(9, 11), P(13, 15)], outline=ACC, width=int(1.3 * SS))


def icon_table(d):
    box(d, 4, 8, 28, 24, w=1.1)
    for y in (14, 19):
        poly(d, [(4, y), (28, y)], col=DIM, w=0.9)
    for x in (10, 16, 22):
        poly(d, [(x, 8), (x, 24)], col=DIM, w=0.9)


def icon_text(d):
    poly(d, [(9, 25), (16, 7)], w=1.6)
    poly(d, [(23, 25), (16, 7)], w=1.6)
    poly(d, [(11.5, 19.5), (20.5, 19.5)], w=1.4)
    square(d, 16, 7, 1.7)


def icon_color(d):
    dot(d, 11, 22, 4.0, GREEN)
    dot(d, 22, 24, 4.0, ACC)


ICONS = [
    ("snap", icon_osnap), ("line", icon_line), ("pline", icon_pline),
    ("arc", icon_arc), ("polygon", icon_polygon), ("rect", icon_rect),
    ("arc2", icon_arc2), ("circle", icon_circle), ("cloud", icon_cloud),
    ("spline", icon_spline), ("ellipse", icon_ellipse), ("ring", icon_ring),
    ("copy", icon_copy), ("move", icon_move), ("point", icon_point),
    ("hatch", icon_hatch), ("image", icon_image), ("block", icon_block),
    ("table", icon_table), ("text", icon_text), ("color", icon_color),
]


VARIANTS = {
    "":   ((208, 217, 228, 255), (62, 156, 245, 255), None),
    "hi": ((255, 255, 255, 255), (94, 178, 255, 255), (46, 80, 120, 255)),
    "on": ((255, 255, 255, 255), (140, 205, 255, 255), (30, 79, 138, 255)),
}


def render(folder, line_col, acc_col, bg):
    global LINE, ACC
    LINE, ACC = line_col, acc_col
    os.makedirs(folder, exist_ok=True)
    for name, fn in ICONS:
        big = Image.new("RGBA", (CW * SS, CH * SS), (0, 0, 0, 0))
        d = ImageDraw.Draw(big)
        if bg is not None:
            d.rounded_rectangle([SS, SS, (CW - 1) * SS, (CH - 1) * SS],
                                radius=3 * SS, fill=bg)
        fn(d)
        img = big.resize((CW, CH), Image.LANCZOS)
        img.save(os.path.join(folder, name + ".png"))


def main():
    for v, colors in VARIANTS.items():
        render(os.path.join(OUT, v) if v else OUT, *colors)
    # contact sheet for review (dark background, logos at 3x)
    sheet_w, sheet_h = 32 * 3 + 24, (32 * 3 + 6) * len(ICONS) + 12
    sheet = Image.new("RGB", (sheet_w, sheet_h), (43, 51, 63))
    y = 6
    for name, _ in ICONS:
        im = Image.open(os.path.join(OUT, name + ".png")).resize((96, 96), Image.LANCZOS)
        sheet.paste(im, (12, y), im)
        y += 102
    sheet.save(r"C:\temp\toolbar_icons_sheet.png")
    print("icons: %d -> %s" % (len(ICONS), OUT))


main()
