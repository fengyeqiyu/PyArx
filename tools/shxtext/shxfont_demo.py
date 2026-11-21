# -*- coding: utf-8 -*-
"""SHX 字体渲染自检：西文 / 中文 / 混合，输出 PNG+SVG 供目视验证。"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import shxfont as F

FD = r"C:\Program Files\Autodesk\AutoCAD 2024\Fonts"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
os.makedirs(OUT, exist_ok=True)


def main():
    romans = F.ShxFont(os.path.join(FD, "romans.shx"))
    hztxt = F.ShxFont(os.path.join(FD, "hztxt.shx"))
    print("romans: kind=%s glyphs=%d" % (romans.kind, len(romans)))
    print("hztxt : kind=%s glyphs=%d" % (hztxt.kind, len(hztxt)))

    for ch in "Ag5":
        g = romans.glyph(ord(ch))
        print("  '%s' -> %r  y=[%.1f,%.1f]" % (ch, g, g.ymin, g.ymax))

    st = F.ShxTextStyle(romans)
    st_cn = F.ShxTextStyle(romans, bigfont=hztxt)

    cases = [
        ("latin", st, "ABCabcGg0189 ~!?"),
        ("cn", st_cn, "图纸建筑设计说明"),
        ("mix", st_cn, "PyArx 图纸 2026"),
    ]
    for name, style, text in cases:
        polys = style.layout(text, height=21.0)
        png = os.path.join(OUT, "shx_%s.png" % name)
        svg = os.path.join(OUT, "shx_%s.svg" % name)
        F.save_png(polys, png, px_per_unit=12, aa=2)
        F.save_svg(polys, svg)
        print("%-6s %-24s polylines=%3d  %s" % (name, text, len(polys), os.path.getsize(png)))


if __name__ == "__main__":
    main()
