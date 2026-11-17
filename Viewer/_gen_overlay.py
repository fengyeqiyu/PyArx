"""把识别结果按世界坐标覆模到画布：每个图元按所属柜框着色（唯一归属/无归属/重叠），不做截图。"""
import json
import os

SCENE = r"G:\PyArx\viewer\web\scene.json"
CAB = r"G:\PyArx\viewer\export\cabinets_all.json"
OUT = r"G:\PyArx\viewer\web\overlay.html"
YMIN = 500000

d = json.load(open(SCENE, encoding="utf-8"))
cabs = json.load(open(CAB, encoding="utf-8"))["cabinets"]
BOX = [c["box"] for c in cabs]


def owner(x, y):
    """返回 >=0 柜序号；-1 无归属；-2 多框重叠"""
    hit = -1
    for j, b in enumerate(BOX):
        if b[0] <= x <= b[2] and b[1] <= y <= b[3]:
            if hit >= 0:
                return -2
            hit = j
    return hit


def cat(o):
    """0=唯一归属 1=无归属 2=重叠"""
    return 0 if o >= 0 else (1 if o == -1 else 2)


L = d["ln"]
HIDE = set(d.get("lyh") or [])          # 变更批注 / 修改痕迹层，默认不画
noise = []
lines = []
for i in range(0, len(L) - 5, 6):
    y1, y2 = L[i + 1], L[i + 3]
    if max(y1, y2) < YMIN:
        continue
    seg = [round(L[i]), round(y1), round(L[i + 2]), round(y2)]
    if L[i + 5] in HIDE:
        noise.append(seg)
        continue
    o = owner((L[i] + L[i + 2]) / 2, (y1 + y2) / 2)
    seg.append(cat(o)); seg.append(o)
    lines.append(seg)

plines = []
for p in d["pl"]:
    raw = p["p"]
    ys = raw[1::2]
    if not ys or max(ys) < YMIN:
        continue
    if p.get("li") in HIDE:
        continue
    m = len(raw) // 4 * 2
    o = owner(raw[m], raw[m + 1])
    plines.append([round(v) for v in raw] + [cat(o), o])

circles = []
for i in range(0, len(d["cr"]) - 4, 5):
    if d["cr"][i + 1] < YMIN:
        continue
    if d["cr"][i + 4] in HIDE:
        continue
    o = owner(d["cr"][i], d["cr"][i + 1])
    circles.append([round(d["cr"][i]), round(d["cr"][i + 1]), round(d["cr"][i + 2]),
                    cat(o), o])

texts = []
for t in d["tx"]:
    if t[1] < YMIN or not (t[6] or "").strip():
        continue
    if t[7] in HIDE:
        continue
    o = owner(t[0], t[1])
    texts.append([round(t[0]), round(t[1]), t[2], t[3], t[4], t[6], cat(o), o, t[8]])

# ---- 块参照展开：把 ins（含嵌套、镜像矩阵）展成世界坐标图元，否则断路器/进线符号整块丢失 ----
import math
BLK = d.get("blk") or {}


def xf(m, x, y):
    return (m[0] * x + m[1] * y + m[4], m[2] * x + m[3] * y + m[5])


def mmul(a, b):
    return [a[0] * b[0] + a[1] * b[2], a[0] * b[1] + a[1] * b[3],
            a[2] * b[0] + a[3] * b[2], a[2] * b[1] + a[3] * b[3],
            a[0] * b[4] + a[1] * b[5] + a[4], a[2] * b[4] + a[3] * b[5] + a[5]]


def emit(name, m, depth):
    if depth > 4 or name not in BLK:
        return
    bd = BLK[name]
    ang = math.degrees(math.atan2(m[2], m[0]))
    sc = math.hypot(m[0], m[2]) or 1.0
    L2 = bd["ln"]
    for i in range(0, len(L2) - 4, 5):
        p1 = xf(m, L2[i], L2[i + 1]); p2 = xf(m, L2[i + 2], L2[i + 3])
        if max(p1[1], p2[1]) < YMIN:
            continue
        o = owner((p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2)
        lines.append([round(p1[0]), round(p1[1]), round(p2[0]), round(p2[1]), cat(o), o])
    for p in bd["pl"]:
        raw = p.get("p") or []
        pts = [xf(m, raw[j], raw[j + 1]) for j in range(0, len(raw) - 1, 2)]
        if not pts or max(q[1] for q in pts) < YMIN:
            continue
        flat = []
        for q in pts:
            flat += [round(q[0]), round(q[1])]
        o = owner(pts[0][0], pts[0][1])
        plines.append(flat + [cat(o), o])
    cr = bd["cr"]
    for i in range(0, len(cr) - 3, 4):
        cc = xf(m, cr[i], cr[i + 1])
        if cc[1] < YMIN:
            continue
        o = owner(*cc)
        circles.append([round(cc[0]), round(cc[1]), round(cr[i + 2] * sc), cat(o), o])
    for t in bd["tx"]:
        pp = xf(m, t[0], t[1])
        if pp[1] < YMIN:
            continue
        o = owner(*pp)
        texts.append([round(pp[0]), round(pp[1]), round(t[2] * sc, 1),
                      round(t[3] + ang, 2), t[4], t[6], cat(o), o, t[7] if len(t) > 7 else 0])
    for sub in bd.get("ins") or []:
        emit(sub["k"], mmul(m, sub["m"]), depth + 1)


nb0 = len(lines)
for b in d["ins"]:
    if b.get("li") in HIDE or b["y"] < YMIN:
        continue
    emit(b["k"], b["m"], 0)
print("块展开新增线 %d 条（总线 %d）" % (len(lines) - nb0, len(lines)))

cnt = {0: 0, 1: 0, 2: 0}
for it in lines:
    cnt[it[4]] += 1
for it in plines:
    cnt[it[-2]] += 1
for it in circles:
    cnt[it[3]] += 1
for it in texts:
    cnt[it[6]] += 1
print("唯一归属 %d / 无归属 %d / 重叠 %d（合计 %d）" % (cnt[0], cnt[1], cnt[2], sum(cnt.values())))

S = {"lines": lines, "plines": plines, "circles": circles, "texts": texts, "noise": noise,
     "boxes": [{"b": c["box"], "n": c["name"], "pe": c["pe"], "lz": len(c["loops"]),
                "w": c["warn"]} for c in cabs], "cnt": cnt}

HTML = r"""<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<title>识别覆模 · 79 台柜</title><style>
html,body{margin:0;height:100%;background:#0e1113;overflow:hidden}
#cv{display:block;width:100%;height:100%}
#hud{position:fixed;left:14px;top:12px;color:#cfd8e3;font:12px/1.75 "Microsoft YaHei",sans-serif;
background:rgba(16,21,26,.9);border:1px solid #2b3742;border-radius:6px;padding:10px 14px;min-width:230px}
#hud b{color:#e9c46a;font-size:13px}
.k{display:inline-block;width:9px;height:9px;border-radius:2px;margin-right:6px;vertical-align:1px}
#tip{position:fixed;right:14px;top:12px;color:#cfd8e3;font:12px/1.7 "Microsoft YaHei",sans-serif;
background:rgba(16,21,26,.9);border:1px solid #2b3742;border-radius:6px;padding:10px 14px;max-width:300px}
#tip b{color:#e9c46a}
#bt{margin-top:9px;padding:4px 13px;background:#1f2a33;color:#cfd8e3;border:1px solid #35434f;
border-radius:5px;cursor:pointer;font:12px "Microsoft YaHei",sans-serif}
#bt:hover{background:#25323d}
.row{margin-top:7px;display:flex;gap:6px}
.row button{flex:1;padding:4px 8px;background:#1f2a33;color:#cfd8e3;border:1px solid #35434f;
border-radius:5px;cursor:pointer;font:12px "Microsoft YaHei",sans-serif}
.row button:hover{background:#25323d}
</style></head><body><canvas id="cv"></canvas>
<div id="hud"><b>识别覆模</b><br>
<span class="k" style="background:#6fae8f"></span>唯一归属 <span id="c1"></span> 个图元<br>
<span class="k" style="background:#4e5a66"></span>无归属（框外漏掉）<span id="c0"></span><br>
<span class="k" style="background:#e07a5f"></span>多框重叠（框串了）<span id="c2"></span><br>
框 <span id="nb"></span> 台 · 滚轮缩放 · 拖动平移 · 按 <b>M</b> 切框 · 按 <b>H</b> 显/隐干扰线（已隐藏 <span id="nn"></span> 条变更批注线）</div>
<div id="tip">点击画布任意图元 →<br><b id="who">（未选）</b><br>
<button id="bt">▶ 逐台走秀</button>
<div class="row"><button id="bex1">导出视图 3x</button><button id="bex2">导出全部</button></div></div>
<script>var S=__DATA__;</script>
<script>
var cv=document.getElementById('cv'),g=cv.getContext('2d');
var W=0,H=0,dpr=1,k=1,x0=0,y1=0,showBox=true,showNoise=false;
var CO={0:'#6fae8f',1:'#4e5a66',2:'#e07a5f'};
document.getElementById('c1').textContent=S.cnt['0'];
document.getElementById('c0').textContent=S.cnt['1'];
document.getElementById('c2').textContent=S.cnt['2'];
document.getElementById('nb').textContent=S.boxes.length;
document.getElementById('nn').textContent=S.noise.length;
// 文字宽度预计算（100px 基准），避免每帧对上万条文字 measureText
var TW=[];
(function(){
  var mc=document.createElement('canvas').getContext('2d');
  mc.font='100px "Microsoft YaHei",sans-serif';
  for(var i=0;i<S.texts.length;i++)TW[i]=mc.measureText(S.texts[i][5]).width;
})();
function fit(){
  var minx=1e12,maxx=-1e12,miny=1e12,maxy=-1e12;
  for(var i=0;i<S.boxes.length;i++){var b=S.boxes[i].b;
    if(b[0]<minx)minx=b[0];if(b[2]>maxx)maxx=b[2];
    if(b[1]<miny)miny=b[1];if(b[3]>maxy)maxy=b[3];}
  k=Math.min(W/((maxx-minx)*1.05),H/((maxy-miny)*1.10));
  x0=(minx+maxx)/2-W/(2*k); y1=(miny+maxy)/2+H/(2*k);
}
var SX=function(x){return (x-x0)*k},SY=function(y){return (y1-y)*k};
function draw(){
  g.setTransform(dpr,0,0,dpr,0,0);
  g.fillStyle='#0e1113';g.fillRect(0,0,W,H);
  var vx0=x0,vx1=x0+W/k,vy1=y1,vy0=y1-H/k;
  // 框
  if(showBox){
    for(var i=0;i<S.boxes.length;i++){
      if(SOLO>=0 && i!==SOLO)continue;
      var b=S.boxes[i].b, lx=SX(b[0]),rx=SX(b[2]),ty=SY(b[3]),by=SY(b[1]);
      g.save();g.setLineDash([6,4]);g.lineWidth=1.2;
      g.strokeStyle=S.boxes[i].w?'#e07a5f':'rgba(233,196,106,.55)';
      g.strokeRect(lx,ty,rx-lx,by-ty);g.restore();
    }
  }
  // 干扰线（变更批注引线），默认不画
  if(showNoise){
    g.strokeStyle='rgba(176,124,196,.6)';g.lineWidth=1;g.beginPath();
    for(var i=0;i<S.noise.length;i++){
      var l=S.noise[i];
      if(Math.max(l[0],l[2])<vx0||Math.min(l[0],l[2])>vx1)continue;
      if(Math.max(l[1],l[3])<vy0||Math.min(l[1],l[3])>vy1)continue;
      g.moveTo(SX(l[0]),SY(l[1]));g.lineTo(SX(l[2]),SY(l[3]));
    }
    g.stroke();
  }
  // 线：按归属着色
  g.lineWidth=1;
  for(var c=0;c<3;c++){
    g.strokeStyle=CO[c];g.beginPath();
    for(var i=0;i<S.lines.length;i++){
      var l=S.lines[i];if(l[4]!==c)continue;
      if(SOLO>=0 && l[5]!==SOLO)continue;
      if(Math.max(l[0],l[2])<vx0||Math.min(l[0],l[2])>vx1)continue;
      if(Math.max(l[1],l[3])<vy0||Math.min(l[1],l[3])>vy1)continue;
      g.moveTo(SX(l[0]),SY(l[1]));g.lineTo(SX(l[2]),SY(l[3]));
    }
    g.stroke();
  }
  for(var c=0;c<3;c++){
    g.strokeStyle=CO[c];
    for(var i=0;i<S.plines.length;i++){
      var p=S.plines[i];if(p[p.length-2]!==c)continue;
      if(SOLO>=0 && p[p.length-1]!==SOLO)continue;
      var n=p.length-2;g.beginPath();
      for(var j=0;j<n;j+=2){var sx=SX(p[j]),sy=SY(p[j+1]);j?g.lineTo(sx,sy):g.moveTo(sx,sy);}
      g.stroke();
    }
    for(var i=0;i<S.circles.length;i++){
      var q=S.circles[i];if(q[3]!==c)continue;
      if(SOLO>=0 && q[4]!==SOLO)continue;
      g.beginPath();g.arc(SX(q[0]),SY(q[1]),Math.max(1,q[2]*k),0,6.2832);g.stroke();
    }
  }
  // 文字：对齐点 + 旋转。用 canvas 自身的 textAlign/textBaseline 落字，
  // 对齐点即 CAD 的 attach（非左对齐时 scan 已换成 alignmentPoint），旋转围绕对齐点。
  for(var c=0;c<3;c++){
    g.fillStyle=c===0?'#dfe6ed':CO[c];
    for(var i=0;i<S.texts.length;i++){
      var t=S.texts[i];if(t[6]!==c)continue;
      if(SOLO>=0 && t[7]!==SOLO)continue;
      var fs=t[2]*k;
      if(fs<3.2)continue;
      if(fs<7)fs=7;else if(fs>30)fs=30;
      var px=SX(t[0]),py=SY(t[1]);
      if(px<-400||px>W+400||py<-150||py>H+150)continue;
      g.save();
      g.font=fs.toFixed(1)+'px "Microsoft YaHei",sans-serif';
      g.translate(px,py);
      if(t[3])g.rotate(-t[3]*Math.PI/180);
      var ha=t[4], va=t[8]||0;
      g.textAlign = (ha===1||ha===4)?'center':(ha===2?'right':'left');
      g.textBaseline = (ha===4||va===2)?'middle':(va===3?'top':(va===1?'bottom':'alphabetic'));
      g.fillText(t[5],0,0);
      g.restore();
    }
  }
}
function resize(){
  dpr=window.devicePixelRatio||1;W=cv.clientWidth;H=cv.clientHeight;
  cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr);draw();
}
function pick(e){
  var wx=x0+e.clientX/k, wy=y1-e.clientY/k;
  for(var i=S.boxes.length-1;i>=0;i--){
    var b=S.boxes[i].b;
    if(wx>=b[0]&&wx<=b[2]&&wy>=b[1]&&wy<=b[3]){
      var o=S.boxes[i];
      document.getElementById('who').textContent=(o.n||'(无名)')+'  Pe='+o.pe+'  回路 '+o.lz+(o.w?'  ⚠ '+o.w:'');
      return;
    }
  }
  document.getElementById('who').textContent='框外 · 世界坐标 '+Math.round(wx)+','+Math.round(wy);
}
var drag=null;cv.addEventListener('mousedown',function(e){drag={x:e.clientX,y:e.clientY,x0:x0,y1:y1,moved:0};});
window.addEventListener('mouseup',function(e){if(drag&&!drag.moved)pick(e);drag=null;});
window.addEventListener('mousemove',function(e){
  if(!drag)return;
  if(Math.abs(e.clientX-drag.x)+Math.abs(e.clientY-drag.y)>3)drag.moved=1;
  x0=drag.x0-(e.clientX-drag.x)/k; y1=drag.y1+(e.clientY-drag.y)/k; draw();
});
cv.addEventListener('wheel',function(e){
  e.preventDefault();var mx=e.clientX,my=e.clientY;
  var wx=x0+mx/k,wy=y1-my/k;k*=(e.deltaY<0?1.18:0.85);
  x0=wx-mx/k;y1=wy+my/k;draw();
},{passive:false});
window.addEventListener('keydown',function(e){
  if(e.key==='m'||e.key==='M'){showBox=!showBox;draw();}
  if(e.key==='h'||e.key==='H'){showNoise=!showNoise;draw();}
});
window.addEventListener('resize',resize);
resize();fit();
// ---- 直接导出 PNG（离屏画布重绘 + toBlob，不经截屏） ----
function offscreen(w,h,ox,oy,ok,odpr){
  var oc=document.createElement('canvas');
  oc.width=Math.max(1,Math.round(w)); oc.height=Math.max(1,Math.round(h));
  var keep=[g,dpr,W,H,k,x0,y1];
  g=oc.getContext('2d'); dpr=odpr; W=oc.width/odpr; H=oc.height/odpr;
  k=ok; x0=ox; y1=oy;
  draw();
  g=keep[0];dpr=keep[1];W=keep[2];H=keep[3];k=keep[4];x0=keep[5];y1=keep[6];
  draw();
  return oc;
}
function savePng(oc,name){
  oc.toBlob(function(b){
    var a=document.createElement('a');
    a.href=URL.createObjectURL(b); a.download=name;
    document.body.appendChild(a); a.click();
    setTimeout(function(){URL.revokeObjectURL(a.href);a.remove();},2000);
    document.getElementById('who').textContent='已导出 '+name+'  '+oc.width+'×'+oc.height;
  },'image/png');
}
document.getElementById('bex1').onclick=function(){
  var s=3;
  savePng(offscreen(W*s,H*s,x0,y1,k,s),'overlay_view_'+Date.now()+'.png');
};
document.getElementById('bex2').onclick=function(){
  var ub=[1e12,1e12,-1e12,-1e12];
  for(var i=0;i<S.boxes.length;i++){var b=S.boxes[i].b;
    if(b[0]<ub[0])ub[0]=b[0]; if(b[1]<ub[1])ub[1]=b[1];
    if(b[2]>ub[2])ub[2]=b[2]; if(b[3]>ub[3])ub[3]=b[3];}
  var pad=1500, uw=ub[2]-ub[0]+pad*2, uh=ub[3]-ub[1]+pad*2;
  var tw=6000, kk=tw/uw;
  savePng(offscreen(tw, uh*kk, ub[0]-pad, ub[3]+pad, kk, 1), 'overlay_all_'+Date.now()+'.png');
};
// ---- 逐台走秀 ----
var TOUR=-1,TOURT=null,STEP=1500,SOLO=-1;
function focusCab(i){
  var b=S.boxes[i].b,w=b[2]-b[0],h=b[3]-b[1];
  k=Math.min(W*0.82/w,H*0.90/h);
  x0=(b[0]+b[2])/2-W/(2*k); y1=(b[1]+b[3])/2+H/(2*k);
  SOLO=i;                       // 只画这台，其余柜的图元/框全部隐藏
  var o=S.boxes[i];
  document.getElementById('who').textContent=
    (i+1)+' / '+S.boxes.length+'  '+(o.n||'(无名)')+'  Pe='+o.pe+'  回路 '+(o.lz||0)+(o.w?'  ⚠ '+o.w:'');
  draw();
}
function tourStep(){
  if(TOUR<0)return;
  focusCab(TOUR);
  document.getElementById('hud').style.opacity='.22';
  TOUR=(TOUR+1)%S.boxes.length;
  TOURT=setTimeout(tourStep,STEP);
}
document.getElementById('bt').onclick=function(){
  if(TOUR>=0){clearTimeout(TOURT);TOUR=-1;SOLO=-1;this.textContent='▶ 逐台走秀';
    document.getElementById('hud').style.opacity='1';draw();return;}
  TOUR=0;this.textContent='⏸ 暂停';tourStep();
};
var hv=location.hash;
if(/^#tour/.test(hv)){
  setTimeout(function(){document.getElementById('bt').click();},80);
}
if(/^#t=/.test(hv)){
  var ti=parseInt(hv.slice(3),10);
  if(isFinite(ti)&&ti>=1&&ti<=S.boxes.length)setTimeout(function(){
    focusCab(ti-1);document.getElementById('hud').style.opacity='.22';},60);
}
if(/^#ex=/.test(hv)){
  var m=hv.slice(4);
  setTimeout(function(){(m==='all'?document.getElementById('bex2'):document.getElementById('bex1')).click();},150);
}

if(/^#b=/.test(hv)){
  var q=hv.slice(3).split(',').map(Number);
  if(q.length===4&&q.every(function(v){return isFinite(v);})){
    k=Math.min(W/((q[2]-q[0])*1.03),H/((q[3]-q[1])*1.03));
    x0=(q[0]+q[2])/2-W/(2*k);y1=(q[1]+q[3])/2+H/(2*k);
  }
}
draw();
</script></body></html>"""

HTML = HTML.replace("__DATA__", json.dumps(S, separators=(",", ":"), ensure_ascii=False))
open(OUT, "w", encoding="utf-8").write(HTML)
print("写出", OUT, os.path.getsize(OUT) // 1024, "KB")
