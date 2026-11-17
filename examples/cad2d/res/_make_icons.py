# -*- coding: utf-8 -*-
"""生成 CAD 工具条图标（32×32，透明底，浅灰线框）。

为什么自己画：XIconButton 的 Image 只吃位图文件，仓库里没有一套 CAD 图标。
风格对齐 AutoCAD 深色主题——单色线框，2px 描边，无填充。
"""
import os

from PIL import Image, ImageDraw

OUT = r"G:\PyArx\examples\cad2d\res"
S = 32
FG = (201, 207, 214, 255)
ACC = (232, 178, 61, 255)


def canvas():
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def save(img, name):
    img.save(os.path.join(OUT, name + ".png"))


def line(d, p1, p2, c=FG, w=2):
    d.line([p1, p2], fill=c, width=w)


def dot(d, p, c=FG, r=2):
    d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=c)


def ic_line():
    img, d = canvas()
    line(d, (5, 26), (27, 6))
    dot(d, (5, 26))
    dot(d, (27, 6))
    return img


def ic_pline():
    img, d = canvas()
    pts = [(5, 26), (12, 13), (19, 20), (27, 6)]
    d.line(pts, fill=FG, width=2)
    for p in (pts[0], pts[-1]):
        dot(d, p)
    return img


def ic_circle():
    img, d = canvas()
    d.ellipse([5, 5, 27, 27], outline=FG, width=2)
    dot(d, (16, 16), r=2)
    return img


def ic_arc():
    img, d = canvas()
    d.arc([6, 10, 30, 34], start=180, end=300, fill=FG, width=2)
    dot(d, (6, 26))
    dot(d, (26, 11))
    return img


def ic_rect():
    img, d = canvas()
    d.rectangle([5, 9, 27, 23], outline=FG, width=2)
    return img


def ic_text():
    img, d = canvas()
    line(d, (6, 8), (26, 8))
    line(d, (16, 8), (16, 26))
    line(d, (10, 26), (22, 26))
    return img


def ic_hatch():
    img, d = canvas()
    d.rectangle([5, 8, 27, 24], outline=FG, width=2)
    for i in range(4):
        line(d, (7 + i * 4, 22), (16 + i * 4, 10))
    return img


def ic_dim():
    img, d = canvas()
    line(d, (5, 10), (5, 22))
    line(d, (27, 10), (27, 22))
    line(d, (5, 16), (27, 16))
    line(d, (10, 13), (5, 16), w=2)
    line(d, (10, 19), (5, 16), w=2)
    line(d, (22, 13), (27, 16), w=2)
    line(d, (22, 19), (27, 16), w=2)
    return img


def ic_erase():
    img, d = canvas()
    line(d, (7, 7), (25, 25))
    line(d, (25, 7), (7, 25))
    return img


def ic_move():
    img, d = canvas()
    line(d, (16, 4), (16, 28))
    line(d, (4, 16), (28, 16))
    for a, b in (((16, 4), (12, 9)), ((16, 4), (20, 9)), ((16, 28), (12, 23)),
                 ((16, 28), (20, 23)), ((4, 16), (9, 12)), ((4, 16), (9, 20)),
                 ((28, 16), (23, 12)), ((28, 16), (23, 20))):
        line(d, a, b)
    return img


def ic_copy():
    img, d = canvas()
    d.rectangle([4, 4, 19, 19], outline=FG, width=2)
    d.rectangle([12, 12, 28, 28], outline=FG, width=2)
    return img


def ic_rotate():
    img, d = canvas()
    d.arc([5, 5, 27, 27], start=200, end=340, fill=FG, width=2)
    line(d, (22, 6), (26, 10))
    line(d, (22, 6), (17, 5))
    dot(d, (16, 16), r=2)
    return img


def ic_fit():
    img, d = canvas()
    d.rectangle([4, 6, 28, 26], outline=FG, width=2)
    line(d, (9, 12), (23, 12))
    line(d, (9, 20), (23, 20))
    line(d, (9, 12), (9, 20))
    line(d, (23, 12), (23, 20))
    return img


def ic_zin():
    img, d = canvas()
    d.ellipse([4, 4, 22, 22], outline=FG, width=2)
    line(d, (20, 20), (28, 28))
    line(d, (13, 8), (13, 18))
    line(d, (8, 13), (18, 13))
    return img


def ic_zout():
    img, d = canvas()
    d.ellipse([4, 4, 22, 22], outline=FG, width=2)
    line(d, (20, 20), (28, 28))
    line(d, (8, 13), (18, 13))
    return img


def ic_grid():
    img, d = canvas()
    d.rectangle([5, 5, 27, 27], outline=FG, width=2)
    line(d, (12, 5), (12, 27))
    line(d, (20, 5), (20, 27))
    line(d, (5, 12), (27, 12))
    line(d, (5, 20), (27, 20))
    return img


def ic_layer():
    img, d = canvas()
    d.line([(5, 11), (16, 5), (27, 11), (16, 17), (5, 11)], fill=FG, width=2)
    d.line([(5, 18), (16, 24), (27, 18)], fill=FG, width=2)
    return img


def ic_save():
    img, d = canvas()
    d.rectangle([6, 5, 26, 27], outline=FG, width=2)
    d.rectangle([11, 20, 21, 27], outline=FG, width=2)
    line(d, (11, 8), (20, 8))
    return img


def ic_export():
    img, d = canvas()
    line(d, (16, 4), (16, 20))
    line(d, (10, 10), (16, 4))
    line(d, (22, 10), (16, 4))
    d.rectangle([6, 20, 26, 28], outline=FG, width=2)
    return img


def ic_select():
    img, d = canvas()
    poly = [(7, 5), (7, 25), (12, 19), (16, 28), (20, 26), (16, 18), (24, 18)]
    d.polygon(poly, outline=FG)
    d.line(poly + [poly[0]], fill=FG, width=2)
    return img

def ic_new():
    img, d = canvas()
    d.line([(8, 4), (19, 4), (26, 11), (26, 28), (8, 28), (8, 4)], fill=FG, width=2)
    line(d, (19, 4), (19, 11))
    line(d, (19, 11), (26, 11))
    return img


def ic_undo():
    img, d = canvas()
    d.arc([8, 10, 28, 28], start=300, end=90, fill=FG, width=2)
    line(d, (8, 14), (11, 9))
    line(d, (8, 14), (4, 10))
    return img


def ic_redo():
    img, d = canvas()
    d.arc([4, 10, 24, 28], start=90, end=240, fill=FG, width=2)
    line(d, (24, 14), (21, 9))
    line(d, (24, 14), (28, 10))
    return img


def ic_point():
    img, d = canvas()
    line(d, (16, 8), (16, 24))
    line(d, (8, 16), (24, 16))
    dot(d, (16, 16), r=3, c=ACC)
    return img


ICONS = {
    "ic_line": ic_line, "ic_pline": ic_pline, "ic_circle": ic_circle, "ic_arc": ic_arc,
    "ic_rect": ic_rect, "ic_text": ic_text, "ic_hatch": ic_hatch, "ic_dim": ic_dim,
    "ic_erase": ic_erase, "ic_move": ic_move, "ic_copy": ic_copy, "ic_rotate": ic_rotate,
    "ic_fit": ic_fit, "ic_zin": ic_zin, "ic_zout": ic_zout, "ic_grid": ic_grid,
    "ic_layer": ic_layer, "ic_save": ic_save, "ic_export": ic_export,
    "ic_select": ic_select, "ic_new": ic_new, "ic_undo": ic_undo, "ic_redo": ic_redo,
    "ic_point": ic_point,
}

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for name, fn in ICONS.items():
        save(fn(), name)
    print("icons:", len(ICONS), "->", OUT)
