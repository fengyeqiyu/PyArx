# -*- coding: utf-8 -*-
"""定位 ODA 静态链接证据 + GL 渲染特征（着色器/纹理/字形）。"""
import re

P = r"G:\PyArx\CAD快速看图\CADReader.exe"
data = open(P, "rb").read()
asc = [m.group().decode("ascii", "ignore") for m in re.finditer(rb"[\x20-\x7e]{4,}", data)]

def ctx(word, n=6, width=3):
    idx = [i for i, s in enumerate(asc) if word in s]
    print("== %s : %d 处" % (word, len(idx)))
    for i in idx[:n]:
        lo, hi = max(0, i - width), min(len(asc), i + width + 1)
        print("   ...", " | ".join(asc[j][:60] for j in range(lo, hi)))

for w in ("Teigha", "DWGdirect", "OdDb", "OdRx", "OdStatic", "OdaMfc", "ODA "):
    ctx(w)

print("\n== GL 特征 ==")
for w in ("gl_Position", "varying", "uniform", "glCreateShader", "glBindBuffer",
          "glDrawArrays", "glDrawElements", "GL_TEXTURE_2D", "glyph", "glyphs",
          "tessellat", "stipple", "linetype", "linetypes", "lineweight", "truecolor"):
    n = sum(1 for s in asc if w in s)
    if n:
        print("  %-18s %d" % (w, n))

print("\n== SHX/字体 ==")
for w in ("shx", ".shx", "SHX", "romans", "gbcbig", "simplex", "hztxt",
          "TTF", "ttf", "Fonts", "fonts", "FontPath"):
    n = sum(1 for s in asc if w in s)
    if n:
        print("  %-12s %d" % (w, n))

print("\n== 目录/资源线索 ==")
for w in ("QResource", "qrc:", ".rcc", ":/", ":/images", ":/icons"):
    n = sum(1 for s in asc if w.startswith("/") == False and w in s) if w.startswith("/") else sum(1 for s in asc if w in s)
    print("  %-12s %d" % (w, n))

print("\n== 关键路径字符串 ==")
for s in asc:
    if len(s) < 5:
        continue
    if re.search(r"[A-Za-z]:\\|\\\\|\.dwg|\.dxf|\.shx|\.ttf|\.pat|\.lin", s, re.I) and len(s) < 80:
        print("   ", s)
