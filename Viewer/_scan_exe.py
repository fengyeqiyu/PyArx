# -*- coding: utf-8 -*-
"""扫 CADReader.exe 的字符串特征，判断 DWG 解析与字体/渲染路线。"""
import re

P = r"G:\PyArx\CAD快速看图\CADReader.exe"
data = open(P, "rb").read()
print("size MB", round(len(data) / 1048576, 1))

asc = re.findall(rb"[\x20-\x7e]{5,}", data)
u16 = re.findall(rb"(?:[\x20-\x7e]\x00){5,}", data)

KEYS = [
    "AC1015", "AC1018", "AC1021", "AC1024", "AC1027", "AC1032", "AC1009",
    "Teigha", "ODA", "libredwg", "LibreDWG", "dwgread", "DWGdirect",
    "AcDbBlockReference", "AcDbAttributeDefinition", "AcDbAttribute",
    "AcDbLine", "AcDbText", "AcDbMText", "AcDbPolyline", "AcDbEntity",
    "SHX", "shx", "simplex", "romans", "txt.shx", "gbcbig", "hztxt",
    "TrueType", "FreeType", "freetype", "fontconfig",
    "Proxy", "PROXY", "AcDbProxyEntity",
    "LWPOLYLINE", "HATCH", "DIMENSION", "SPLINE", "MLINE",
    "GL_ARB", "glGenBuffers", "glDraw", "VBO", "FBO",
    "QGLWidget", "QGraphicsView", "QWebView",
    "libxl", "xlsx", "pdf",
]

def scan(name, chunks):
    found = {}
    for c in chunks:
        s = c.decode("ascii", "ignore")
        for k in KEYS:
            if k in s:
                found[k] = found.get(k, 0) + 1
    return found

a = scan("ascii", asc)
b = scan("utf16", u16)
merged = {}
for d in (a, b):
    for k, v in d.items():
        merged[k] = merged.get(k, 0) + v

for k in KEYS:
    if k in merged:
        print("%-24s hits=%d" % (k, merged[k]))

print("--- 疑似 ACAD 类名字符串样本 ---")
cls = sorted({c.decode('ascii', 'ignore') for c in asc
              if c.decode('ascii', 'ignore').startswith('AcDb')})
print(len(cls))
for c in cls[:40]:
    print("  ", c)
