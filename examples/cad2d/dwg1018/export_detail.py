# -*- coding: utf-8 -*-
"""局部细节：固定框内几何 -> XScene XML，供 MFC 画布放大查看。"""
import sys, math, json, io
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import render_svg

OUT = r"G:\PyArx\examples\cad2d\dwg1018\dwg_detail.xml"
REF_SCENE = r"G:\PyArx\viewer\web\scene.json"

# 局部框（CAD 原始坐标，y 向上）
BX = (150000.0, -60000.0, 400000.0, 0.0)
xmin, ymin, xmax, ymax = BX

elems, n = render_svg.collect_expanded()
COL = json.load(io.open(REF_SCENE, encoding="utf-8"))["c"]

def in_box(x, y):
    return xmin <= x <= xmax and ymin <= y <= ymax

def colhex(c):
    try: return COL[int(c)]
    except Exception: return "#111111"

def esc(t):
    return (t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;"))

def f(v): return f"{v:.2f}"

L = ["<XScene>"]
cnt = {"line":0,"circle":0,"arc":0,"poly":0,"text":0}

for x1,y1,x2,y2,c in elems["line"]:
    if in_box(x1,y1) and in_box(x2,y2):
        L.append(f'<XLine X1="{f(x1)}" Y1="{f(-y1)}" X2="{f(x2)}" Y2="{f(-y2)}" Stroke="{colhex(c)}"/>'); cnt["line"]+=1
for cx,cy,r,c in elems["circle"]:
    if in_box(cx,cy):
        L.append(f'<XCircle Cx="{f(cx)}" Cy="{f(-cy)}" R="{f(r)}" Stroke="{colhex(c)}"/>'); cnt["circle"]+=1

def arc_pts(cx,cy,r,sa,ea):
    nseg=max(8,min(48,int(r/15)+8)); o=[]
    for i in range(nseg+1):
        t=sa+(ea-sa)*i/nseg; o.append((cx+r*math.cos(t),cy+r*math.sin(t)))
    return o

for cx,cy,r,sa,ea,c in elems["arc"]:
    if in_box(cx,cy):
        pts=arc_pts(cx,cy,r,sa,ea)
        if all(in_box(x,y) for x,y in pts):
            L.append(f'<XPolyline Points="{" ".join(f"{f(x)} {f(-y)}" for x,y in pts)}" Stroke="{colhex(c)}"/>'); cnt["arc"]+=1

for pts,closed,c in elems["poly"]:
    if all(in_box(x,y) for x,y in pts):
        L.append(f'<XPolyline Points="{" ".join(f"{f(x)} {f(-y)}" for x,y in pts)}" Closed="{1 if closed else 0}" Stroke="{colhex(c)}"/>'); cnt["poly"]+=1
for pts,c in elems["spline"]:
    if len(pts)>=2 and all(in_box(x,y) for x,y in pts):
        L.append(f'<XPolyline Points="{" ".join(f"{f(x)} {f(-y)}" for x,y in pts)}" Stroke="{colhex(c)}"/>'); cnt["poly"]+=1
for cx,cy,major,ratio,sa,ea,c in elems["ellipse"]:
    mx,my=major[0],major[1]; mr=math.hypot(mx,my) or 1.0
    ux,uy=mx/mr,my/mr; vx,vy=-uy,ux; rmin=mr*ratio
    nseg=max(12,min(64,int(mr/20))); pts=[]
    for i in range(nseg+1):
        t=sa+(ea-sa)*i/nseg; dx=mr*math.cos(t); dy=rmin*math.sin(t)
        pts.append((cx+dx*ux+dy*vx, cy+dx*uy+dy*vy))
    if all(in_box(x,y) for x,y in pts):
        L.append(f'<XPolyline Points="{" ".join(f"{f(x)} {f(-y)}" for x,y in pts)}" Stroke="{colhex(c)}"/>'); cnt["poly"]+=1
for x,y,hgt,rot,t,c in elems["text"]:
    if in_box(x,y):
        t=t.replace(r"\P","").strip()
        if not t: continue
        L.append(f'<XText X="{f(x)}" Y="{f(-y)}" Size="{f(max(hgt,1.0))}" Color="{colhex(c)}" Rotate="{f(-math.degrees(rot))}">{esc(t)}</XText>'); cnt["text"]+=1

L.append("</XScene>")
io.open(OUT,"w",encoding="utf-8").write("\n".join(L))
print(cnt, "box", BX, "out", OUT)
