# -*- coding: utf-8 -*-
"""看 Gs 设备实现 / 显示列表 / 缓存 / 文字折线化 的自研类。"""
import re

P = r"G:\PyArx\CAD快速看图\CADReader.exe"
data = open(P, "rb").read()
raw = re.findall(rb"\.\?A[VU]([A-Za-z_][\x20-\x7e]{2,90}?)@@", data)
uniq = sorted({n.decode("ascii", "ignore") for n in raw})

def show(tag, words, limit=45):
    hit = [s for s in uniq if any(w.lower() in s.lower() for w in words)]
    print("\n== %s : %d ==" % (tag, len(hit)))
    for s in hit[:limit]:
        print("   ", s[:95])

show("Gs 设备/视图实现", ["GsDevice", "GsView", "Vectoriz", "GsBaseVector", "OdGi"])
show("显示列表/缓存/瓦片", ["DisplayList", "Dsp", "Tile", "Cache", "LOD", "Preprocess", "Prepare"])
show("文字折线化/轮廓", ["PolyDraw", "ProcFunc", "OutLine", "Contour", "Stroke", "Bezier"])
show("自研场景对象", ["SceneObj", "DrawingItem", "DrawingScene", "ViewPort", "RenderItem"])
show("OpenGL 侧自研", ["GL", "QGL", "glWidget", "QOpenGL"])
