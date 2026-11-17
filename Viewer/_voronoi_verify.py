"""Voronoi 归属：每个图元归属到距离最近的那张参数表，边界由内容自然决定，消除矩形框重叠。"""
import json
import re

SCENE = r"G:\PyArx\viewer\web\scene.json"
OUTJ = r"G:\PyArx\viewer\export\cab_voronoi.json"
OUTH = r"G:\PyArx\viewer\web\voronoi.html"
YMIN = 500000
NAME = re.compile(r"\d{3}-\d+-\d{0,2}[A-Z]{2,}\d{0,2}")

d = json.load(open(SCENE, encoding="utf-8"))
TB = [t["b"] for t in d["tb"]]


def dbox(x, y, b):
    dx = 0 if b[0] <= x <= b[2] else min(abs(x - b[0]), abs(x - b[2]))
    dy = 0 if b[1] <= y <= b[3] else min(abs(y - b[1]), abs(y - b[3]))
    return (dx * dx + dy * dy) ** 0.5


def nearest(x, y):
    best, bd = -1, 1e18
    for i, b in enumerate(TB):
        v = dbox(x, y, b)
        if v < bd:
            bd, best = v, i
            if v == 0:
                break
    return best


L = d["ln"]
lines = []
for i in range(0, len(L) - 5, 6):
    y1, y2 = L[i + 1], L[i + 3]
    if max(y1, y2) < YMIN:
        continue
    mx, my = (L[i] + L[i + 2]) / 2, (y1 + y2) / 2
    lines.append([round(L[i]), round(y1), round(L[i + 2]), round(y2), nearest(mx, my)])

plines = []
for p in d["pl"]:
    raw = p["p"]
    ys = raw[1::2]
    if not ys or max(ys) < YMIN:
        continue
    m = len(raw) // 4 * 2
    plines.append([round(v) for v in raw] + [nearest(raw[m], raw[m + 1])])

circles = []
for i in range(0, len(d["cr"]) - 4, 5):
    if d["cr"][i + 1] < YMIN:
        continue
    circles.append([round(d["cr"][i]), round(d["cr"][i + 1]), round(d["cr"][i + 2]),
                    nearest(d["cr"][i], d["cr"][i + 1])])

texts = []
for t in d["tx"]:
    if t[1] < YMIN or not (t[6] or "").strip():
        continue
    texts.append([round(t[0]), round(t[1]), t[2], t[6], nearest(t[0], t[1])])

# 每台柜的内容范围
grp = {i: {"n": 0, "x0": 1e18, "x1": -1e18, "y0": 1e18, "y1": -1e18} for i in range(len(TB))}
for arr in (lines, plines, circles, texts):
    for it in arr:
        t = it[-1]
        if arr is plines:
            gx, gy = it[0], it[1]
        else:
            gx, gy = it[0], it[1]
        q = grp[t]
        q["n"] += 1
        q["x0"] = min(q["x0"], gx); q["x1"] = max(q["x1"], gx)
        q["y0"] = min(q["y0"], gy); q["y1"] = max(q["y1"], gy)

names = {}
for t in texts:
    if NAME.match(t[3].strip()) and 250 <= t[2] <= 340:
        k = t[4]
        if k not in names or t[1] > names[k][1]:
            names[k] = (t[3], t[1])

print("=== Voronoi 归属结果 ===")
ns = sorted(g["n"] for g in grp.values())
print("  每台图元数 中位 %d（%d~%d）" % (ns[len(ns) // 2], ns[0], ns[-1]))
ws = sorted(grp[i]["x1"] - grp[i]["x0"] for i in range(len(TB)))
hs = sorted(grp[i]["y1"] - grp[i]["y0"] for i in range(len(TB)))
print("  内容宽 中位 %d（%d~%d）" % (ws[len(ws) // 2], ws[0], ws[-1]))
print("  内容高 中位 %d（%d~%d）" % (hs[len(hs) // 2], hs[0], hs[-1]))
print("  有柜名 %d / %d" % (len(names), len(TB)))
print("  图元数 < 20 的台：%s" % [i + 1 for i in range(len(TB)) if grp[i]["n"] < 20])
for i in range(min(8, len(TB))):
    g = grp[i]
    print("  #%-3d %-18s 图元%3d  宽%6d 高%6d  内容框 %d,%d,%d,%d"
          % (i + 1, names.get(i, ("—", 0))[0], g["n"], g["x1"] - g["x0"], g["y1"] - g["y0"],
             g["x0"], g["y0"], g["x1"], g["y1"]))

out = []
for i in range(len(TB)):
    g = grp[i]
    out.append({"i": i + 1, "name": names.get(i, ("", 0))[0], "table": TB[i],
                "n": g["n"],
                "bbox": [round(g["x0"]), round(g["y0"]), round(g["x1"]), round(g["y1"])]
                if g["n"] else None})
json.dump(out, open(OUTJ, "w", encoding="utf-8"), ensure_ascii=False, indent=1)

S = {"lines": lines, "plines": plines, "circles": circles, "texts": texts,
     "boxes": [{"b": o["bbox"] or TB[i], "n": o["name"], "n2": o["n"]} for i, o in enumerate(out)]}
HTML = r"""<!doctype html><html><head><meta charset="utf-8"><title>Voronoi 归属</title><style>
html,body{margin:0;height:100%;background:#0e1113;overflow:hidden}#cv{display:block;width:100%;height:100%}
#hud{position:fixed;left:14px;top:12px;color:#cfd8e3;font:12px/1.7 "Microsoft YaHei",sans-serif;
background:rgba(16,21,26,.9);border:1px solid #2b3742;border-radius:6px;padding:10px 14px}
#hud b{color:#e9c46a}</style></head><body><canvas id="cv"></canvas>
<div id="hud"><b>Voronoi 归属（每图元归最近表）</b><br>绿框=该表实际内容范围 · 每台图元数见点选</div>
<script>var S=__DATA__;</script><script>
var cv=document.getElementById('cv'),g=cv.getContext('2d'),W=0,H=0,dpr=1,k=1,x0=0,y1=0;
function fit(){var a=1e12,b=-1e12,c=1e12,e=-1e12;
for(var i=0;i<S.boxes.length;i++){var b2=S.boxes[i].b;if(!b2)continue;
if(b2[0]<a)a=b2[0];if(b2[2]>b)b=b2[2];if(b2[1]<c)c=b2[1];if(b2[3]>e)e=b2[3];}
k=Math.min(W/((b-a)*1.05),H/((e-c)*1.1));x0=(a+b)/2-W/(2*k);y1=(c+e)/2+H/(2*k);}
var SX=function(x){return (x-x0)*k},SY=function(y){return (y1-y)*k};
function draw(){g.setTransform(dpr,0,0,dpr,0,0);g.fillStyle='#0e1113';g.fillRect(0,0,W,H);
g.lineWidth=1;g.strokeStyle='#5d6b78';g.beginPath();
for(var i=0;i<S.lines.length;i++){var l=S.lines[i];g.moveTo(SX(l[0]),SY(l[1]));g.lineTo(SX(l[2]),SY(l[3]));}
g.stroke();g.strokeStyle='#4e7a63';g.beginPath();
for(var i=0;i<S.circles.length;i++){var q=S.circles[i];g.moveTo(SX(q[0]+q[2]),SY(q[1]));g.arc(SX(q[0]),SY(q[1]),Math.max(1,q[2]*k),0,6.2832);}
g.stroke();
for(var i=0;i<S.boxes.length;i++){var b=S.boxes[i].b;if(!b)continue;
var lx=SX(b[0]),rx=SX(b[2]),ty=SY(b[3]),by=SY(b[1]);
g.strokeStyle='rgba(111,174,143,.65)';g.lineWidth=1;g.strokeRect(lx,ty,rx-lx,by-ty);}
for(var i=0;i<S.texts.length;i++){var t=S.texts[i],fs=t[2]*k;if(fs<4)continue;
var px=SX(t[0]),py=SY(t[1]);if(px<-60||px>W+60||py<-20||py>H+20)continue;
g.fillStyle='#dfe6ed';g.font=Math.min(fs,26).toFixed(1)+'px "Microsoft YaHei",sans-serif';g.fillText(t[3],px,py);}
}
function rz(){dpr=window.devicePixelRatio||1;W=cv.clientWidth;H=cv.clientHeight;
cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr);draw();}
var dr=null;cv.addEventListener('mousedown',function(e){dr={x:e.clientX,y:e.clientY,x0:x0,y1:y1};});
window.addEventListener('mouseup',function(){dr=null;});
window.addEventListener('mousemove',function(e){if(!dr)return;
x0=dr.x0-(e.clientX-dr.x)/k;y1=dr.y1+(e.clientY-dr.y)/k;draw();});
cv.addEventListener('wheel',function(e){e.preventDefault();var mx=e.clientX,my=e.clientY;
var wx=x0+mx/k,wy=y1-my/k;k*=(e.deltaY<0?1.18:0.85);x0=wx-mx/k;y1=wy+my/k;draw();},{passive:false});
window.addEventListener('resize',rz);rz();fit();draw();
</script></body></html>"""
open(OUTH, "w", encoding="utf-8").write(HTML.replace("__DATA__", json.dumps(S, separators=(",", ":"), ensure_ascii=False)))
print("写出", OUTH)
