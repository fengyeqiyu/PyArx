"""
cabvision —— 柜体视觉识别模型（不依赖 PIL / OpenCV / scipy）

管线：
  1. rasterize : 矢量图元 -> 二值位图（轴对齐段才画，斜引线不参与）
  2. runs      : 逐行/逐列行进扫描，抽出水平 / 垂直结构线；允许 gap 内合并断线
  3. fit_rects : 上下横线 + 左右竖线拟合闭合矩形
  4. filter    : 按柜体尺寸筛，输出每台柜的世界坐标框

用法：
  python cabvision.py                      # 打印识别结果
  python cabvision.py --html cuts.html     # 同时生成可视化核对页
"""
import argparse
import json
import os
from collections import defaultdict

import numpy as np

SCENE = r"G:\PyArx\viewer\web\scene.json"
YMIN = 500000        # 只看系统图带（平面图在 y < 50 万）
CELL = 150           # 世界单位 / 像素
GAP_H = 4            # 横线合并容差(像素) 4*150=600 世界单位（必须小于柜内行距 1168）
GAP_V = 4
MIN_H = 80           # 横线最短(像素) 80*150=12000
MIN_V = 100          # 竖线最短 100*150=15000
XTOL = 4             # 同一台柜各行线的端点一致容差(像素) 4*150=600
YTOL = 8             # 同一台柜内行线归属的纵向容差(像素)


def rasterize(d, cell=CELL, ymin=YMIN):
    """把轴对齐图元画进二值位图。返回 (G, X0, YLO) ；G[y, x] 对应世界 (X0+x*cell, YLO+y*cell)。"""
    L = d["ln"]
    xs, ys = [], []
    for i in range(0, len(L), 6):
        xs.extend((L[i], L[i + 2]))
        ys.extend((L[i + 1], L[i + 3]))
    X0, X1 = min(xs), max(xs)
    YLO, YHI = ymin, max(ys)
    COLS, ROWS = int((X1 - X0) / cell) + 3, int((YHI - YLO) / cell) + 3
    G = np.zeros((ROWS, COLS), np.uint8)

    def seg(x1, y1, x2, y2):
        """只画水平/垂直段；一个像素即可，runs 阶段靠合并补连续。"""
        if abs(y2 - y1) <= 40:                      # 水平
            a, b = sorted((int((x1 - X0) / cell), int((x2 - X0) / cell)))
            r = int((y1 - YLO) / cell)
            if 0 <= r < ROWS:
                G[r, max(0, a):min(COLS, b + 1)] = 1
        elif abs(x2 - x1) <= 40:                    # 垂直
            a, b = sorted((int((y1 - YLO) / cell), int((y2 - YLO) / cell)))
            c = int((x1 - X0) / cell)
            if 0 <= c < COLS:
                G[max(0, a):min(ROWS, b + 1), c] = 1

    for i in range(0, len(L) - 5, 6):
        if max(L[i + 1], L[i + 3]) < ymin:
            continue
        seg(L[i], L[i + 1], L[i + 2], L[i + 3])

    for p in d["pl"]:
        raw = p["p"]
        if not raw or max(raw[1::2]) < ymin:
            continue
        for j in range(0, len(raw) - 3, 2):
            seg(raw[j], raw[j + 1], raw[j + 2], raw[j + 3])
    return G, X0, YLO


def runs_1d(bit, gap, minlen):
    """一维行进扫描：连续置位段，间隔 <= gap 视为同一条线，长度 >= minlen 才输出。"""
    idx = np.flatnonzero(bit)
    if idx.size == 0:
        return []
    out = []
    s = p = int(idx[0])
    for i in idx[1:]:
        i = int(i)
        if i - p <= gap + 1:
            p = i
        else:
            if p - s + 1 >= minlen:
                out.append((s, p))
            s = p = i
    if p - s + 1 >= minlen:
        out.append((s, p))
    return out


def scan(G, axis):
    """axis=0 逐行找横线；axis=1 逐列找竖线。返回 [(固定坐标, 起, 止)]。"""
    n = G.shape[0] if axis == 0 else G.shape[1]
    gap, minlen = (GAP_H, MIN_H) if axis == 0 else (GAP_V, MIN_V)
    out = []
    for i in range(n):
        line = G[i] if axis == 0 else G[:, i]
        for a, b in runs_1d(line, gap, minlen):
            out.append((i, a, b))
    return out


def fit_rects(hor, ver, cell=CELL):
    """上下横线配对 + 左右竖线验证，拟合闭合矩形（像素坐标）。"""
    R = defaultdict(list)
    for y, a, b in hor:
        R[a // 20].append((y, a, b))       # 按 x 起点粗桶，减少比较
    rects = []
    tall_v = [v for v in ver if v[2] - v[1] >= 30000 / cell]      # 只留能当侧边的竖线
    for k, hs in R.items():
        hs.sort()
        for i in range(len(hs)):
            for j in range(i + 1, len(hs)):
                y1, a1, b1 = hs[i]
                y2, a2, b2 = hs[j]
                if not (1 <= (y2 - y1) * cell <= 56000):
                    continue
                if abs(a1 - a2) > 3 or abs(b1 - b2) > 3:             # 上下边端点需对齐(450 世界单位)
                    continue
                xl, xr = (a1 + a2) / 2, (b1 + b2) / 2
                # 左右侧边
                v_l = [v for v in tall_v if abs(v[0] - xl) <= 2 and v[1] * cell <= y1 * cell + 800 and v[2] >= y2 - 1]
                v_r = [v for v in tall_v if abs(v[0] - xr) <= 2 and v[1] * cell <= y1 * cell + 800 and v[2] >= y2 - 1]
                if v_l and v_r:
                    rects.append((round(xl), y1, round(xr), y2))
    return rects


def dedupe(rects):
    seen, out = set(), []
    for r in sorted(rects, key=lambda t: (t[2] - t[0]) * (t[3] - t[1])):
        key = (r[0] // 6, r[1] // 6, r[2] // 6, r[3] // 6)
        if key in seen:
            continue
        seen.add(key)
        out.append(r)
    return out


def cluster_rows(hor, cell, X0, YLO):
    """柜体判据：一组端点一致(x 容差 XTOL)、纵向连续(y 容差 YTOL)、行数>=5 的水平结构线。"""
    lines = sorted([(a, b, y) for y, a, b in hor], key=lambda t: -t[2])
    cl = []
    for a, b, y in lines:
        for c in cl:
            if (abs(a - c["a0"]) <= XTOL and abs(b - c["a1"]) <= XTOL
                    and c["ylo"] - YTOL <= y <= c["yhi"] + YTOL):
                c["a0"] = min(c["a0"], a)
                c["a1"] = max(c["a1"], b)
                c["ylo"] = min(c["ylo"], y)
                c["yhi"] = max(c["yhi"], y)
                c["n"] += 1
                break
        else:
            cl.append({"a0": a, "a1": b, "ylo": y, "yhi": y, "n": 1})

    cab = []
    for c in cl:
        if c["n"] < 5:
            continue
        x0, x1 = X0 + c["a0"] * cell, X0 + c["a1"] * cell
        y0, y1 = YLO + c["ylo"] * cell, YLO + c["yhi"] * cell
        w, h = x1 - x0, y1 - y0
        if 17000 <= w <= 36000 and 27000 <= h <= 56000:
            cab.append({"b": [round(x0), round(y0), round(x1), round(y1)],
                        "w": round(w), "h": round(h), "rows": c["n"]})
    return cab


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scene", default=SCENE)
    ap.add_argument("--html", default="")
    ap.add_argument("--cell", type=int, default=CELL)
    a = ap.parse_args()

    d = json.load(open(a.scene, encoding="utf-8"))
    G, X0, YLO = rasterize(d, a.cell)
    print("位图 %d x %d  非零点 %d  (cell=%d)" % (G.shape[1], G.shape[0], int(G.sum()), a.cell))

    hor = scan(G, 0)
    ver = scan(G, 1)
    print("横结构线 %d 条   竖结构线 %d 条" % (len(hor), len(ver)))

    rects = dedupe(fit_rects(hor, ver, a.cell))
    print("闭合矩形 %d 个（供参考）" % len(rects))

    cab = cluster_rows(hor, a.cell, X0, YLO)
    cab.sort(key=lambda c: (-c["b"][3], c["b"][0]))
    print("柜体量级 %d 个" % len(cab))
    if cab:
        ws = sorted(c["w"] for c in cab)
        hs = sorted(c["h"] for c in cab)
        rs = sorted(c.get("rows", 0) for c in cab)
        print("宽 中位 %d [%d~%d]   高 中位 %d [%d~%d]   行数 中位 %d [%d~%d]"
              % (ws[len(ws) // 2], ws[0], ws[-1], hs[len(hs) // 2], hs[0], hs[-1],
                 rs[len(rs) // 2], rs[0], rs[-1]))
        for c in cab[:14]:
            print("   %s  %dx%d  %d行" % (c["b"], c["w"], c["h"], c.get("rows", 0)))
    json.dump(cab, open(r"G:\PyArx\viewer\export\cab_vision.json", "w", encoding="utf-8"),
              ensure_ascii=False, indent=1)

    if a.html:
        build_html(d, cab, a.html)


def build_html(d, cab, out):
    YM = YMIN
    lines, plines, circles, texts = [], [], [], []
    L = d["ln"]
    for i in range(0, len(L) - 5, 6):
        if max(L[i + 1], L[i + 3]) >= YM:
            lines.extend([round(L[i]), round(L[i + 1]), round(L[i + 2]), round(L[i + 3])])
    for p in d["pl"]:
        raw = p["p"]
        if raw and max(raw[1::2]) >= YM:
            plines.append([round(v) for v in raw])
    for i in range(0, len(d["cr"]) - 4, 5):
        if d["cr"][i + 1] >= YM:
            circles.extend([round(d["cr"][i]), round(d["cr"][i + 1]), round(d["cr"][i + 2])])
    for t in d["tx"]:
        if t[1] >= YM and (t[6] or "").strip():
            texts.append([round(t[0]), round(t[1]), t[2], t[6]])

    S = {"lines": lines, "plines": plines, "circles": circles, "texts": texts,
         "boxes": [c["b"] for c in cab]}
    tpl = """<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<title>视觉识别核对</title><style>
html,body{margin:0;height:100%;background:#0B0E11;overflow:hidden}
#cv{display:block;width:100%;height:100%}
#hud{position:fixed;left:14px;top:12px;color:#cfd8e3;font:12px/1.7 "Microsoft YaHei",sans-serif;
background:rgba(18,24,30,.88);border:1px solid #2b3742;border-radius:6px;padding:10px 14px}
#hud b{color:#e9c46a;font-size:13px}
</style></head><body><canvas id="cv"></canvas>
<div id="hud"><b>视觉识别核对</b><br>
框 = 横线行进扫描 + 竖线拟合出的闭合矩形<br>
框数 <span id="n">0</span> · 滚轮缩放 · 拖动平移 · 点框看尺寸</div>
<script>var S=__DATA__;</script>
<script>
var cv=document.getElementById('cv'),g=cv.getContext('2d');
var W=0,H=0,dpr=1,k=1,x0=0,y1=0;
document.getElementById('n').textContent=S.boxes.length;
var SX=function(x){return (x-x0)*k},SY=function(y){return (y1-y)*k};
function fit(){
  var B=S.boxes;if(!B.length)return;
  var a=1e9,b=-1e9,c=1e9,d=-1e9;
  for(var i=0;i<B.length;i++){a=Math.min(a,B[i][0]);b=Math.max(b,B[i][2]);c=Math.min(c,B[i][1]);d=Math.max(d,B[i][3]);}
  k=Math.min(W/((b-a)*1.05),H/((d-c)*1.12));x0=(a+b)/2-W/(2*k);y1=(c+d)/2+H/(2*k);
}
function draw(){
  g.setTransform(dpr,0,0,dpr,0,0);
  g.fillStyle='#0B0E11';g.fillRect(0,0,W,H);
  var vx0=x0,vx1=x0+W/k,vy1=y1,vy0=y1-H/k;
  g.lineWidth=1;g.strokeStyle='#6E7B87';g.beginPath();
  var L=S.lines;
  for(var i=0;i<L.length;i+=4){
    if(Math.max(L[i],L[i+2])<vx0||Math.min(L[i],L[i+2])>vx1)continue;
    if(Math.max(L[i+1],L[i+3])<vy0||Math.min(L[i+1],L[i+3])>vy1)continue;
    g.moveTo(SX(L[i]),SY(L[i+1]));g.lineTo(SX(L[i+2]),SY(L[i+3]));
  }
  g.stroke();
  g.strokeStyle='#57A98A';
  for(var i=0;i<S.plines.length;i++){
    var p=S.plines[i];g.beginPath();
    for(var j=0;j<p.length;j+=2){var sx=SX(p[j]),sy=SY(p[j+1]);j?g.lineTo(sx,sy):g.moveTo(sx,sy);}
    g.stroke();
  }
  g.strokeStyle='#8FB3CC';var C=S.circles;
  for(var i=0;i<C.length;i+=3){g.beginPath();g.arc(SX(C[i]),SY(C[i+1]),Math.max(1,C[i+2]*k),0,6.2832);g.stroke();}
  g.fillStyle='#DFE7EE';g.font='10px "Microsoft YaHei",sans-serif';
  for(var i=0;i<S.texts.length;i++){
    var t=S.texts[i],px=SX(t[0]),py=SY(t[1]);
    if(px<-40||px>W+40||py<-20||py>H+20)continue;
    g.fillText(t[3],px,py);
  }
  for(var i=0;i<S.boxes.length;i++){
    var b=S.boxes[i],lx=SX(b[0]),rx=SX(b[2]),ty=SY(b[3]),by=SY(b[1]);
    g.fillStyle='rgba(233,196,106,.07)';g.fillRect(lx,ty,rx-lx,by-ty);
    g.strokeStyle='#E9C46A';g.lineWidth=1.5;g.strokeRect(lx,ty,rx-lx,by-ty);
    g.fillStyle='#E9C46A';g.font='11px "Microsoft YaHei",sans-serif';
    g.fillText(String(i+1),lx+3,ty+12);
  }
}
function resize(){dpr=window.devicePixelRatio||1;W=cv.clientWidth;H=cv.clientHeight;
cv.width=Math.round(W*dpr);cv.height=Math.round(H*dpr);draw();}
var drag=null,hint=document.getElementById('hud');
cv.addEventListener('mousedown',function(e){drag={x:e.clientX,y:e.clientY,x0:x0,y1:y1};});
window.addEventListener('mouseup',function(){drag=null;});
window.addEventListener('mousemove',function(e){if(!drag)return;
x0=drag.x0-(e.clientX-drag.x)/k;y1=drag.y1+(e.clientY-drag.y)/k;draw();});
cv.addEventListener('wheel',function(e){e.preventDefault();
var mx=e.clientX,my=e.clientY,wx=x0+mx/k,wy=y1-my/k;k*=(e.deltaY<0?1.18:0.85);
x0=wx-mx/k;y1=wy+my/k;draw();},{passive:false});
cv.addEventListener('click',function(e){
var wx=x0+e.clientX/k,wy=y1-e.clientY/k,hit=-1;
for(var i=0;i<S.boxes.length;i++){var b=S.boxes[i];
 if(wx>=b[0]&&wx<=b[2]&&wy>=b[1]&&wy<=b[3]){hit=i;break;}}
if(hit>=0){var b=S.boxes[hit];
 hint.innerHTML='<b>#'+(hit+1)+'</b> x '+b[0]+'..'+b[2]+'<br>y '+b[1]+'..'+b[3]+
 '<br>宽 '+(b[2]-b[0])+'  高 '+(b[3]-b[1])+'<br><span style="color:#8b98a5">框数 '+S.boxes.length+' · 滚轮缩放 · 拖动平移</span>';}
});
window.addEventListener('resize',resize);resize();fit();
var hv=location.hash;
if(/^#b=/.test(hv)){var q=hv.slice(3).split(',').map(Number);
 if(q.length===4&&q.every(function(v){return isFinite(v);})){
   var bw=q[2]-q[0],bh=q[3]-q[1];
   k=Math.min(W/(bw*1.04),H/(bh*1.04));x0=(q[0]+q[2])/2-W/(2*k);y1=(q[1]+q[3])/2+H/(2*k);}}
draw();
</script></body></html>"""
    html = tpl.replace("__DATA__", json.dumps(S, separators=(",", ":"), ensure_ascii=False))
    open(out, "w", encoding="utf-8").write(html)
    print("写出 %s  %d KB" % (out, os.path.getsize(out) // 1024))


if __name__ == "__main__":
    main()
