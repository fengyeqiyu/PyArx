"""每张参数表取左侧 5.5 万内出现最多的水平行线组作为该柜真实范围，并画到画布上。"""
import json
from collections import Counter

SCENE = r"G:\PyArx\viewer\web\scene.json"
OUT = r"G:\PyArx\viewer\web\cuts.html"

d = json.load(open(SCENE, encoding="utf-8"))
YMIN = 500000

H = []
L = d["ln"]
for i in range(0, len(L) - 5, 6):
    x1, y1, x2, y2 = L[i], L[i + 1], L[i + 2], L[i + 3]
    if abs(y1 - y2) <= 40 and abs(x1 - x2) >= 3000:
        H.append((round(min(x1, x2)), round(max(x1, x2)), round((y1 + y2) / 2)))
print("水平线:", len(H))

boxes = []
for ti, tb in enumerate(d["tb"]):
    bb = tb["b"]
    tx0, ty0, ty1 = bb[0], bb[1], bb[3]
    pe = ""
    for row in (tb.get("t") or []):
        cells = [str(c) if c is not None else "" for c in row]
        if len(cells) >= 2 and cells[0].strip() == "Pe=":
            pe = cells[1].strip()
    near = [h for h in H
            if tx0 - 55000 <= h[1] <= tx0 + 2000 and ty0 - 55000 <= h[2] <= ty1 + 55000]
    if not near:
        boxes.append({"b": [round(tx0), round(ty0), round(tx0) + 21000, round(ty1) + 38000],
                      "pe": pe, "src": "fallback", "n": 0})
        continue
    key = Counter((h[0], h[1]) for h in near).most_common(1)[0][0]
    grp = [h for h in near if (h[0], h[1]) == key]
    xs = [key[0], key[1]]
    ys = [h[2] for h in grp]
    boxes.append({"b": [xs[0], min(ys), xs[1], max(ys)], "pe": pe, "src": "rows", "n": len(grp)})

ok = [b for b in boxes if b["src"] == "rows"]
print("行线定界:", len(ok), " 回落:", len(boxes) - len(ok))
ws = sorted(b["b"][2] - b["b"][0] for b in ok)
hs = sorted(b["b"][3] - b["b"][1] for b in ok)
if ws:
    print("宽 中位 %d [%d~%d]  高 中位 %d [%d~%d]"
          % (ws[len(ws) // 2], ws[0], ws[-1], hs[len(hs) // 2], hs[0], hs[-1]))
for b in boxes[:8]:
    print("  Pe=%-9s %s  %dx%d  %s(%d行)" % (b["pe"], b["b"], b["b"][2] - b["b"][0],
                                            b["b"][3] - b["b"][1], b["src"], b["n"]))

# ---- 生成画布 ----
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
     "boxes": [{"b": b["b"], "pe": b["pe"], "t": "rows" if b["src"] == "rows" else "est"} for b in boxes]}
print("图元:", len(lines) // 4, "线", len(plines), "多段线", len(circles) // 3, "圆", len(texts), "字",
      len(S["boxes"]), "框")

HTML = """<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<title>识别区域核对</title><style>
html,body{margin:0;height:100%;background:#12161A;overflow:hidden}
#cv{display:block;width:100%;height:100%}
#hud{position:fixed;left:14px;top:12px;color:#cfd8e3;font:12px/1.7 "Microsoft YaHei",sans-serif;
background:rgba(20,26,32,.86);border:1px solid #2b3742;border-radius:6px;padding:10px 14px}
#hud b{color:#e9c46a;font-size:13px}
</style></head><body><canvas id="cv"></canvas>
<div id="hud"><b>识别区域核对</b><br>
黄实框 = 由柜内水平行线定界（结构得到）<br>
橙虚线 = 没找到行线、按表估算<br>
框数 <span id="n">0</span> · 滚轮缩放 · 拖动平移</div>
<script>var S=__DATA__;</script>
<script>
var cv=document.getElementById('cv'),g=cv.getContext('2d');
var W=0,H=0,dpr=1,k=1,x0=0,y1=0;
document.getElementById('n').textContent=S.boxes.length;
function fit(){
  var xs=[],ys=[],B=S.boxes;
  for(var i=0;i<B.length;i++){xs.push(B[i].b[0],B[i].b[2]);ys.push(B[i].b[1],B[i].b[3]);}
  var a=Math.min.apply(0,xs),b=Math.max.apply(0,xs),c=Math.min.apply(0,ys),d=Math.max.apply(0,ys);
  k=Math.min(W/((b-a)*1.05),H/((d-c)*1.10));
  x0=(a+b)/2-W/(2*k);y1=(c+d)/2+H/(2*k);
}
var SX=function(x){return (x-x0)*k},SY=function(y){return (y1-y)*k};
function draw(){
  g.setTransform(dpr,0,0,dpr,0,0);
  g.fillStyle='#12161A';g.fillRect(0,0,W,H);
  g.lineWidth=1;g.strokeStyle='#7f8c99';
  var L=S.lines,vx0=x0,vx1=x0+W/k,vy1=y1,vy0=y1-H/k;
  g.beginPath();
  for(var i=0;i<L.length;i+=4){
    if(Math.max(L[i],L[i+2])<vx0||Math.min(L[i],L[i+2])>vx1)continue;
    if(Math.max(L[i+1],L[i+3])<vy0||Math.min(L[i+1],L[i+3])>vy1)continue;
    g.moveTo(SX(L[i]),SY(L[i+1]));g.lineTo(SX(L[i+2]),SY(L[i+3]));
  }
  g.stroke();
  g.strokeStyle='#6fae8f';
  for(var i=0;i<S.plines.length;i++){
    var p=S.plines[i];g.beginPath();
    for(var j=0;j<p.length;j+=2){var sx=SX(p[j]),sy=SY(p[j+1]);j?g.lineTo(sx,sy):g.moveTo(sx,sy);}
    g.stroke();
  }
  g.strokeStyle='#9ab6cc';
  var C=S.circles;
  for(var i=0;i<C.length;i+=3){g.beginPath();g.arc(SX(C[i]),SY(C[i+1]),Math.max(1,C[i+2]*k),0,6.2832);g.stroke();}
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
    if(bx.t==='rows'){g.setLineDash([]);g.strokeStyle='#e9c46a';g.lineWidth=1.5;}
    else{g.setLineDash([6,5]);g.strokeStyle='#e07a5f';g.lineWidth=1.3;}
    g.strokeRect(lx,ty,rx-lx,by-ty);
    g.restore();
    g.fillStyle=bx.t==='rows'?'rgba(233,196,106,.10)':'rgba(224,122,95,.10)';
    g.fillRect(lx,ty,rx-lx,by-ty);
    g.fillStyle=bx.t==='rows'?'#e9c46a':'#e07a5f';g.font='11px "Microsoft YaHei",sans-serif';
    g.fillText(String(i+1)+' '+bx.pe+'kW',lx+3,ty+12);
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
open(OUT, "w", encoding="utf-8").write(HTML)
import os
print("写出", OUT, round(os.path.getsize(OUT) / 1024), "KB")
json.dump({"file": d["f"], "boxes": boxes},
          open(r"G:\PyArx\viewer\export\cabinets.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
