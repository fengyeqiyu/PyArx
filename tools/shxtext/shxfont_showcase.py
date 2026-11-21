# -*- coding: utf-8 -*-
"""SHX 字体渲染能力演示：多行、字高、宽度因子、倾斜、对齐。"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import shxfont as F

FD = r"C:\Program Files\Autodesk\AutoCAD 2024\Fonts"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")


def rows():
    west = F.ShxFont(os.path.join(FD, "romans.shx"))
    cn = F.ShxFont(os.path.join(FD, "hztxt.shx"))
    st = F.ShxTextStyle(west, bigfont=cn)
    all_polys = []
    h = 21.0
    lines = [
        ("SHX single-line font 0123456789", {}),
        ("图纸设计 建筑说明 结构", {}),
        ("Tilted 15° text", {"oblique": 15.0}),
        ("Width factor 0.75", {"width_factor": 0.75}),
    ]
    y = 0.0
    for text, kw in lines:
        polys = st.layout(text, height=h, origin=(0.0, y), **kw)
        all_polys.extend(polys)
        y -= h * 1.8
    return all_polys


def main():
    os.makedirs(OUT, exist_ok=True)
    polys = rows()
    png = os.path.join(OUT, "shx_showcase.png")
    svg = os.path.join(OUT, "shx_showcase.svg")
    F.save_png(polys, png, px_per_unit=14, aa=2)
    F.save_svg(polys, svg)
    print("polylines=%d  bbox=%s" % (len(polys), tuple(round(v, 1) for v in F.polyline_bbox(polys))))
    print("png=%s (%d bytes)" % (png, os.path.getsize(png)))
    print("svg=%s (%d bytes)" % (svg, os.path.getsize(svg)))


if __name__ == "__main__":
    main()
