"""以 79 张参数表为锚点，用最近邻把每个图元归到唯一一台柜，取归属图元的分位边界作为该柜范围。
这样柜与柜的划分由"离哪张表最近"这一条可复现规则决定，不依赖任何猜测的矩形。"""
import json
import os
from collections import defaultdict

import numpy as np

SCENE = r"G:\PyArx\viewer\web\scene.json"
YMIN = 500000
MAXR = 160000          # 超过这个距离的图元不归任何柜（标题栏、图外内容）
OUTJSON = r"G:\PyArx\viewer\export\all_cabs.json"
OUTHTML = r"G:\PyArx\viewer\web\cabs.html"

d = json.load(open(SCENE, encoding="utf-8"))
tb = d["tb"]
print("参数表:", len(tb))
st = np.array([[(t["b"][0] + t["b"][2]) / 2, (t["b"][1] + t["b"][3]) / 2] for t in tb], float)

# ---- 收集图元代表点 ----
pts, meta = [], []


def add(x, y, kind, ref):
    if y < YMIN:
        return
    pts.append((x, y))
    meta.append((kind, ref))


L = d["ln"]
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if max(y1, y2) < YMIN:
        continue
    if abs(x2 - x1) > 42000 or abs(y2 - y1) > 42000:
        continue
    add((x1 + x2) / 2, (y1 + y2) / 2, "line", (x1, y1, x2, y2))

for p in d["pl"]:
    raw = p["p"]
    if not raw or max(raw[1::2]) < YMIN:
        continue
    for j in range(0, len(raw) - 3, 2):
        add((raw[j] + raw[j + 2]) / 2, (raw[j + 1] + raw[j + 3]) / 2, "pline", None)

for i in range(0, len(d["cr"]) - 4, 5):
    add(d["cr"][i], d["cr"][i + 1], "circle", None)
for i in range(0, len(d["ar"]) - 6, 7):
    add(d["ar"][i], d["ar"][i + 1], "arc", None)
for t in d["tx"]:
    if (t[6] or "").strip():
        add(t[0], t[1], "text", t[6])
for b in d["ins"]:
    add(b["x"], b["y"], "block", b.get("k"))

P = np.array(pts, float)
print("图元点:", len(P))

# ---- 自动识别建筑图例表并排除（这类大表不是配电柜，会撑大相邻柜的框）----
EXCL = []
seeds = []
for t in d["tx"]:
    s = (t[6] or "").replace(" ", "")
    if s in ("图例", "图例说明", "图例表"):
        seeds.append((t[0], t[1], s))
        EXCL.append((t[0] - 3000, t[0] + 32000, t[1] - 42000, t[1] + 3000))
print("图例表种子:", [(round(a), round(b), c) for a, b, c in seeds])

# ---- 最近邻归属 ----
D = np.sqrt(((P[:, None, :] - st[None, :, :]) ** 2).sum(axis=2))
near = D.argmin(axis=1)
dist = D.min(axis=1)
near = np.where(dist <= MAXR, near, -1)

own = {i: {"nums": [], "texts": [], "kinds": {}} for i in range(len(tb))}
for i, s in enumerate(near):
    if s < 0:
        continue
    if abs(P[i][1] - st[s][1]) > 62000:
        continue
    if any(a - 800 <= P[i][0] <= b + 800 and c - 800 <= P[i][1] <= dd + 800 for a, b, c, dd in EXCL):
        continue
    kind, ref = meta[i]
    own[int(s)]["nums"].append(P[i])
    own[int(s)]["kinds"][kind] = own[int(s)]["kinds"].get(kind, 0) + 1
    if kind == "text" and isinstance(ref, str) and ref.strip():
        own[int(s)]["texts"].append(ref.strip())

print("未归属图元:", int((near < 0).sum()))

cabs = []
for i, t in enumerate(tb):
    o = own[i]
    pe = ijs = kx = ""
    for row in (t.get("t") or []):
        c = [str(v) if v is not None else "" for v in row]
        if len(c) >= 2 and c[0].strip() == "Pe=":
            pe = c[1].strip()
        if len(c) >= 2 and c[0].strip() == "Ijs=":
            ijs = c[1].strip()
        if len(c) >= 2 and c[0].strip() == "Kx=":
            kx = c[1].strip()
    bb = t["b"]
    if o["nums"]:
        A = np.array(o["nums"])
        x0, x1 = np.percentile(A[:, 0], [2, 98])
        y0, y1 = np.percentile(A[:, 1], [2, 98])
        box = [round(float(x0)), round(float(y0)), round(float(x1)), round(float(y1))]
        box = [min(box[0], round(bb[0])), min(box[1], round(bb[1])),
               max(box[2], round(bb[2])), max(box[3], round(bb[3]))]
    else:
        box = [round(bb[0]), round(bb[1]), round(bb[0]) + 21000, round(bb[1]) + 40000]
    cabs.append({"i": i, "box": box, "pe": pe, "ijs": ijs, "kx": kx,
                 "n": len(o["nums"]), "kinds": o["kinds"], "texts": o["texts"],
                 "table": [round(v) for v in bb]})

cabs.sort(key=lambda c: (-c["box"][3], c["box"][0]))
ws = [c["box"][2] - c["box"][0] for c in cabs]
hs = [c["box"][3] - c["box"][1] for c in cabs]
print("\n柜数:", len(cabs))
print("宽 中位 %d [%d~%d]   高 中位 %d [%d~%d]"
      % (sorted(ws)[len(ws) // 2], min(ws), max(ws), sorted(hs)[len(hs) // 2], min(hs), max(hs)))
big = [c for c in cabs if c["box"][2] - c["box"][0] > 60000 or c["box"][3] - c["box"][1] > 80000]
print("异常大框(>6万宽或>8万高):", len(big))
for c in big[:6]:
    print("   Pe=%s %s  %dx%d  %d图元" % (c["pe"], c["box"], c["box"][2] - c["box"][0],
                                       c["box"][3] - c["box"][1], c["n"]))
print("\n前 10 台:")
for c in cabs[:10]:
    print("   Pe=%-9s Ijs=%-9s %s  %dx%d  %d图元 %s"
          % (c["pe"], c["ijs"], c["box"], c["box"][2] - c["box"][0], c["box"][3] - c["box"][1],
             c["n"], c["kinds"]))

json.dump({"file": d["f"], "rule": "nearest-table (Voronoi)", "cabs": cabs},
          open(OUTJSON, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("写出", OUTJSON, round(os.path.getsize(OUTJSON) / 1024), "KB")

# ---- 画到画布上供人眼核验 ----
lines, plines, circles, texts = [], [], [], []
for i in range(0, len(L) - 5, 6):
    if max(L[i + 1], L[i + 3]) >= YMIN:
        lines.extend([round(L[i]), round(L[i + 1]), round(L[i + 2]), round(L[i + 3])])
for p in d["pl"]:
    raw = p["p"]
    if raw and max(raw[1::2]) >= YMIN:
        plines.append([round(v) for v in raw])
for i in range(0, len(d["cr"]) - 4, 5):
    if d["cr"][i + 1] >= YMIN:
        circles.extend([round(d["cr"][i]), round(d["cr"][i + 1]), round(d["cr"][i + 2])])
for t in d["tx"]:
    if t[1] >= YMIN and (t[6] or "").strip():
        texts.append([round(t[0]), round(t[1]), t[2], t[6]])

S = {"lines": lines, "plines": plines, "circles": circles, "texts": texts,
     "boxes": [{"b": c["box"], "pe": c["pe"], "ijs": c["ijs"], "n": c["n"],
                "big": (c["box"][2] - c["box"][0] > 60000 or c["box"][3] - c["box"][1] > 80000),
                "tx": c["texts"][:14]} for c in cabs]}
print("画布图元:", len(lines) // 4, "线", len(plines), "多段线", len(circles) // 3, "圆", len(texts), "字")
print("框数:", len(S["boxes"]), " 异常:", sum(1 for b in S["boxes"] if b["big"]))

HTML = """<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<title>柜识别结果</title><style>
html,body{margin:0;height:100%;background:#0E1215;overflow:hidden}
#cv{display:block;width:100%;height:100%}
#hud{position:fixed;left:14px;top:12px;color:#cfd8e3;font:12px/1.7 "Microsoft YaHei",sans-serif;
background:rgba(18,23,28,.88);border:1px solid #2b3742;border-radius:6px;padding:10px 14px;max-width:520px}
#hud b{color:#e9c46a;font-size:13px}
#info{position:fixed;right:14px;top:12px;color:#cfd8e3;font:12px/1.7 "Microsoft YaHei",sans-serif;
background:rgba(18,23,28,.9);border:1px solid #33414d;border-radius:6px;padding:10px 14px;
max-width:320px;display:none}
</style></head><body><canvas id="cv"></canvas>
<div id="hud"><b>柜识别结果（最近邻归属）</b><br>
每框 = 归到该参数表的全部图元范围<br>
黄框 79 台 · 点框看该柜参数与文字<br>
滚轮缩放 · 拖动平移</div>
<div id="info"></div>
<script>var S=__DATA__;</script>
<script>
var cv=document.getElementById('cv'),g=cv.getContext('2d');
var W=0,H=0,dpr=1,k=1,x0=0,y1=0;
function fit(){
  var xs=[],ys=[];
  for(var i=0;i<S.boxes.length;i++){xs.push(S.boxes[i].b[0],S.boxes[i].b[2]);ys.push(S.boxes[i].b[1],S.boxes[i].b[3]);}
  var a=Math.min.apply(0,xs),b=Math.max.apply(0,xs),c=Math.min.apply(0,ys),dd=Math.max.apply(0,ys);
  k=Math.min(W/((b-a)*1.04),H/((dd-c)*1.08));
  x0=(a+b)/2-W/(2*k);y1=(c+dd)/2+H/(2*k);
}
var SX=function(x){return (x-x0)*k},SY=function(y){return (y1-y)*k};
function draw(){
  g.setTransform(dpr,0,0,dpr,0,0);
  g.fillStyle='#0E1215';g.fillRect(0,0,W,H);
  var vx0=x0,vx1=x0+W/k,vy1=y1,vy0=y1-H/k;
  g.lineWidth=1;g.strokeStyle='#7d8b98';
  var Ln=S.lines,gp=g;
  gp.beginPath();
  for(var i=0;i<Ln.length;i+=4){
    if(Math.max(Ln[i],Ln[i+2])<vx0||Math.min(Ln[i],Ln[i+2])>vx1)continue;
    if(Math.max(Ln[i+1],Ln[i+3])<vy0||Math.min(Ln[i+1],Ln[i+3])>vy1)continue;
    gp.moveTo(SX(Ln[i]),SY(Ln[i+1]));gp.lineTo(SX(Ln[i+2]),SY(Ln[i+3]));
  }
  gp.stroke();
  g.strokeStyle='#6fae8f';
  for(var i=0;i<S.plines.length;i++){
    var p=S.plines[i];g.beginPath();
    for(var j=0;j<p.length;j+=2){var sx=SX(p[j]),sy=SY(p[j+1]);j?g.lineTo(sx,sy):g.moveTo(sx,sy);}
    g.stroke();
  }
  g.strokeStyle='#9ab6cc';
  for(var i=0;i<S.circles.length;i+=3){
    var r=Math.max(1,S.circles[i+2]*k);g.beginPath();
    g.arc(SX(S.circles[i]),SY(S.circles[i+1]),r,0,6.2832);g.stroke();
  }
  g.fillStyle='#d6dee6';g.font='10px "Microsoft YaHei",sans-serif';
  for(var i=0;i<S.texts.length;i++){
    var t=S.texts[i],px=SX(t[0]),py=SY(t[1]);
    if(px<-40||px>W+40||py<-20||py>H+20)continue;
    g.fillText(t[3],px,py);
  }
  for(var i=0;i<S.boxes.length;i++){
    var bx=S.boxes[i],b=bx.b;
    var lx=SX(b[0]),rx=SX(b[2]),ty=SY(b[3]),by=SY(b[1]);
    g.save();
    if(bx.big){g.setLineDash([7,5]);g.strokeStyle='#e07a5f';g.lineWidth=1.4;}
    else{g.setLineDash([]);g.strokeStyle='#e9c46a';g.lineWidth=1.4;}
    g.strokeRect(lx,ty,rx-lx,by-ty);
    g.restore();
    g.fillStyle=bx.big?'rgba(224,122,95,.12)':'rgba(233,196,106,.10)';
    g.fillRect(lx,ty,rx-lx,by-ty);
    g.fillStyle=bx.big?'#e07a5f':'#e9c46a';g.font='11px "Microsoft YaHei",sans-serif';
    g.fillText(String(i+1)+' Pe='+bx.pe+'kW',lx+3,ty+12);
  }
}
function resize(){
  dpr=window.devicePixelRatio||1;W=cv.clientWidth;H=cv.clientHeight;
  cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr);draw();
}
var drag=null;
cv.addEventListener('mousedown',function(e){drag={x:e.clientX,y:e.clientY,x0:x0,y1:y1};});
window.addEventListener('mouseup',function(){drag=null;});
window.addEventListener('mousemove',function(e){
  if(!drag)return;x0=drag.x0-(e.clientX-drag.x)/k;y1=drag.y1+(e.clientY-drag.y)/k;draw();
});
cv.addEventListener('click',function(e){
  var wx=x0+e.clientX/k,wy=y1-e.clientY/k,hit=-1;
  for(var i=0;i<S.boxes.length;i++){
    var b=S.boxes[i].b;
    if(wx>=b[0]&&wx<=b[2]&&wy>=b[1]&&wy<=b[3]){
      var a=(b[2]-b[0])*(b[3]-b[1]);
      if(hit<0||a<(S.boxes[hit].b[2]-S.boxes[hit].b[0])*(S.boxes[hit].b[3]-S.boxes[hit].b[1]))hit=i;
    }
  }
  var el=document.getElementById('info');
  if(hit<0){el.style.display='none';return;}
  var bx=S.boxes[hit];
  el.style.display='block';
  el.innerHTML='<b style="color:#e9c46a">'+(hit+1)+' · Pe='+bx.pe+'kW</b><br>Ijs='+bx.ijs+' A'
    +'<br>图元 '+bx.n+' 个<br>框 '+bx.b[0]+','+bx.b[1]+' → '+bx.b[2]+','+bx.b[3]
    +'<br><span style="color:#8fa3b3">文字样本</span><br>'+bx.tx.map(function(s){
      return s.replace(/</g,'&lt;');}).join('<br>');
});
cv.addEventListener('wheel',function(e){
  e.preventDefault();var mx=e.clientX,my=e.clientY;
  var wx=x0+mx/k,wy=y1-my/k;k*=(e.deltaY<0?1.18:0.85);
  x0=wx-mx/k;y1=wy+my/k;draw();
},{passive:false});
window.addEventListener('resize',resize);
resize();fit();
var hv=location.hash;
if(/^#b=/.test(hv)){
  var q=hv.slice(3).split(',').map(Number);
  if(q.length===4&&q.every(function(v){return isFinite(v);})){
    var bw=q[2]-q[0],bh=q[3]-q[1];
    k=Math.min(W/(bw*1.04),H/(bh*1.04));
    x0=(q[0]+q[2])/2-W/(2*k);y1=(q[1]+q[3])/2+H/(2*k);
  }
}
draw();
</script></body></html>"""

HTML = HTML.replace("__DATA__", json.dumps(S, separators=(",", ":"), ensure_ascii=False))
open(OUTHTML, "w", encoding="utf-8").write(HTML)
print("写出", OUTHTML, round(os.path.getsize(OUTHTML) / 1024), "KB")
