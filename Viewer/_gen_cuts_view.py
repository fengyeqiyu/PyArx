"""把 79 台柜的识别框画到画布上：内嵌系统图带图元 + 框，生成自包含 HTML。"""
import json

SCENE = r"G:\PyArx\viewer\web\scene.json"
OUT = r"G:\PyArx\viewer\web\cuts.html"

d = json.load(open(SCENE, encoding="utf-8"))
ly = d.get("ly", [])
YMIN = 500000

# 只保留系统图带，缩减体积
lines, plines, circles, texts = [], [], [], []
L = d["ln"]
for i in range(0, len(L) - 5, 6):
    if max(L[i + 1], L[i + 3]) >= YMIN:
        lines.extend([round(L[i]), round(L[i + 1]), round(L[i + 2]), round(L[i + 3])])
for p in d["pl"]:
    raw = p["p"]
    ys = raw[1::2]
    if ys and max(ys) >= YMIN:
        plines.append([round(v) for v in raw])
for i in range(0, len(d["cr"]) - 4, 5):
    if d["cr"][i + 1] >= YMIN:
        circles.extend([round(d["cr"][i]), round(d["cr"][i + 1]), round(d["cr"][i + 2])])
for t in d["tx"]:
    if t[1] >= YMIN and (t[6] or "").strip():
        texts.append([round(t[0]), round(t[1]), t[2], t[6]])

# 框：表位于柜左下角，向右 2.2 万、向上 4.4 万
boxes = []
for tb in d["tb"]:
    bb = tb["b"]
    pe = ""
    for row in (tb.get("t") or []):
        cells = [str(c) if c is not None else "" for c in row]
        if len(cells) >= 2 and cells[0].strip() == "Pe=":
            pe = cells[1].strip()
    boxes.append({"b": [round(bb[0]), round(bb[1]), round(bb[0]) + 21000, round(bb[1]) + 40000], "pe": pe})

S = {"lines": lines, "plines": plines, "circles": circles, "texts": texts, "boxes": boxes}
print("图元:", len(lines) // 4, "线", len(plines), "多段线", len(circles) // 3, "圆", len(texts), "字", len(boxes), "框")

HTML = """<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<title>识别区域核对 · 79 台柜</title><style>
html,body{margin:0;height:100%;background:#12161A;overflow:hidden}
#cv{display:block;width:100%;height:100%}
#hud{position:fixed;left:14px;top:12px;color:#cfd8e3;font:12px/1.7 "Microsoft YaHei",sans-serif;
background:rgba(20,26,32,.86);border:1px solid #2b3742;border-radius:6px;padding:10px 14px}
#hud b{color:#e9c46a;font-size:13px}
#hud span{color:#8fb8de}
</style></head><body><canvas id="cv"></canvas>
<div id="hud"><b>识别区域核对</b><br>
黄色虚线框 = 判定的一台柜（表位于框左下角）<br>
框数 <span id="n">0</span> · 滚轮缩放 · 拖动平移<br>
<span>点框看该柜 Pe 值</span></div>
<script>var S=__DATA__;</script>
<script>
var cv=document.getElementById('cv'),g=cv.getContext('2d');
var W=0,H=0,dpr=1,k=1,x0=0,y1=0;
document.getElementById('n').textContent=S.boxes.length;
function fit(){
  var xs=[],ys=[];
  for(var i=0;i<S.boxes.length;i++){var b=S.boxes[i].b;xs.push(b[0],b[2]);ys.push(b[1],b[3]);}
  var minx=Math.min.apply(0,xs),maxx=Math.max.apply(0,xs);
  var miny=Math.min.apply(0,ys),maxy=Math.max.apply(0,ys);
  var w=maxx-minx,h=maxy-miny;
  k=Math.min(W/(w*1.06),H/(h*1.10));
  x0=(minx+maxx)/2-W/(2*k); y1=(miny+maxy)/2+H/(2*k);
}
var SX=function(x){return (x-x0)*k},SY=function(y){return (y1-y)*k};
function draw(){
  g.setTransform(dpr,0,0,dpr,0,0);
  g.fillStyle='#12161A';g.fillRect(0,0,W,H);
  g.lineWidth=1;
  g.strokeStyle='#7f8c99';
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
  for(var i=0;i<S.circles.length;i+=3){
    var c=S.circles;g.beginPath();g.arc(SX(c[i]),SY(c[i+1]),Math.max(1,c[i+2]*k),0,6.2832);g.stroke();
  }
  g.fillStyle='#d6dee6';g.font='10px "Microsoft YaHei",sans-serif';
  for(var i=0;i<S.texts.length;i++){
    var t=S.texts[i],px=SX(t[0]),py=SY(t[1]);
    if(px<-40||px>W+40||py<-20||py>H+20)continue;
    g.fillText(t[3],px,py);
  }
  for(var i=0;i<S.boxes.length;i++){
    var b=S.boxes[i].b;
    var lx=SX(b[0]),rx=SX(b[2]),ty=SY(b[3]),by=SY(b[1]);
    g.save();
    g.setLineDash([7,5]);g.lineWidth=1.6;g.strokeStyle='#e9c46a';
    g.strokeRect(lx,ty,rx-lx,by-ty);
    g.restore();
    g.fillStyle='rgba(233,196,106,.16)';g.fillRect(lx,ty,rx-lx,by-ty);
    g.fillStyle='#e9c46a';g.font='11px "Microsoft YaHei",sans-serif';
    g.fillText(String(i+1)+'  '+S.boxes[i].pe+'kW',lx+3,ty+12);
  }
}
function resize(){
  dpr=window.devicePixelRatio||1;
  W=cv.clientWidth;H=cv.clientHeight;
  cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr);
  draw();
}
var drag=null;
cv.addEventListener('mousedown',function(e){drag={x:e.clientX,y:e.clientY,x0:x0,y1:y1};});
window.addEventListener('mouseup',function(){drag=null;});
window.addEventListener('mousemove',function(e){
  if(!drag)return;
  x0=drag.x0-(e.clientX-drag.x)/k; y1=drag.y1+(e.clientY-drag.y)/k; draw();
});
cv.addEventListener('wheel',function(e){
  e.preventDefault();
  var mx=e.clientX,my=e.clientY;
  var wx=x0+mx/k, wy=y1-my/k;
  k*=(e.deltaY<0?1.18:0.85);
  x0=wx-mx/k; y1=wy+my/k; draw();
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
