# -*- coding: utf-8 -*-
"""生成 wpowerplane.html —— 配电柜拓扑图（Canvas2D，读 el_links.json 的 upstream/links）。"""
import json

L = json.load(open(r'G:\PyArx\viewer\export\el_links.json', encoding='utf-8'))
nodes = [{'id': n['id'], 'pe': n.get('pe') or 0, 'n': len(n.get('outLines') or []),
          'inl': n['inl'] if n.get('inl') is not None else len(n.get('inLines') or []),
          'src': n.get('src') or 0}
         for n in L['nodes'] if n['id']]
links = [{'f': x['from'], 't': x['to'], 'v': x.get('via', '')} for x in L['links']]

ids = {n['id'] for n in nodes}
srcs = sorted({l['f'] for l in links if l['f'] not in ids})
for s in srcs:
    nodes.append({'id': s, 'pe': 0, 'n': 0, 'inl': 1, 'src': 1})
children = {}
for l in links:
    children.setdefault(l['f'], []).append(l['t'])
par = {}
for l in links:
    par.setdefault(l['t'], []).append(l['f'])
dep = {}
for n in nodes:
    dep[n['id']] = 0 if (n.get('src') or not par.get(n['id'])) else 999
for _ in range(30):
    changed = False
    for l in links:
        a = dep.get(l['f'], 999)
        if a < 999:
            nd = min(a + 1, 998)
            if nd < dep.get(l['t'], 999):
                dep[l['t']] = nd
                changed = True
    if not changed:
        break
for n in nodes:
    n['dep'] = 999 if n['inl'] == 0 else dep.get(n['id'], 0)

DATA = json.dumps({'nodes': nodes, 'links': links}, ensure_ascii=False)

HTML = r'''<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<title>配电拓扑 · wpowerplane</title>
<style>
html,body{margin:0;height:100%;background:#06080a;color:#dfe8ee;font:12px/1.45 "Segoe UI",Arial,sans-serif;overflow:hidden}
#hud{position:fixed;left:12px;top:10px;z-index:9;color:#8fabbc;pointer-events:none;font-size:12px}
#hud b{color:#eaf4fa}
#bar{position:fixed;right:12px;top:10px;z-index:9;display:flex;gap:6px}
button{background:#11181e;color:#a9c6d8;border:1px solid #254250;border-radius:5px;padding:4px 9px;font-size:11px;cursor:pointer}
button:hover{background:#1a2530;color:#e8f4fa}
button.on{background:#1d3b4a;color:#effaff;border-color:#4a8fae}
canvas{display:block;cursor:grab}
#tip{position:fixed;left:12px;bottom:12px;z-index:9;color:#6d8b9c;font-size:11px;pointer-events:none}
</style>
</head>
<body>
<div id="hud"></div>
<div id="bar">
  <button id="bFit">适应全图</button>
  <button id="bIso" class="on">显孤岛</button>
</div>
<div id="tip">点柜高亮上下游 · 拖拽平移 · 滚轮缩放</div>
<canvas id="cv"></canvas>
<script>
const D = __DATA__;
const cv = document.getElementById('cv'), g = cv.getContext('2d');
const hud = document.getElementById('hud');
let view = {x: 40, y: 20, k: 0.62}, sel = null, showIso = 1;

const id2n = {}, children = {}, parents = {}, pos = {};
D.nodes.forEach(n => id2n[n.id] = n);
D.links.forEach(l => {
  (children[l.f] = children[l.f] || []).push(l.t);
  (parents[l.t] = parents[l.t] || []).push(l.f);
});

let maxD = 0;
const layers = {};
D.nodes.forEach(n => {
  const d = n.dep;
  if (d !== 999 && d > maxD) maxD = d;
  (layers[d] = layers[d] || []).push(n);
});
const layerKeys = Object.keys(layers).map(Number).sort((a, b) => a - b);

const NW = 180, NH = 54, COLX = 262, ROWY = 62;
function layout() {
  const isIso = n => n.inl === 0 && !parents[n.id] && !(children[n.id] || []).length;
  const placed = new Set();
  let leafIdx = 0, colOff = 0, gMaxX = 0;
  function place(id, depth) {
    if (placed.has(id)) return pos[id] ? pos[id].y : 0;
    placed.add(id);
    const ch = (children[id] || []).filter(c => !placed.has(c) && id2n[c]);
    if (!ch.length) {
      pos[id] = { x: colOff + depth * COLX, y: leafIdx * ROWY };
      if (pos[id].x + NW > gMaxX) gMaxX = pos[id].x + NW;
      leafIdx++;
      return pos[id].y;
    }
    const ys = ch.map(c => place(c, depth + 1));
    const y = (Math.min(...ys) + Math.max(...ys)) / 2;
    pos[id] = { x: colOff + depth * COLX, y };
    if (pos[id].x + NW > gMaxX) gMaxX = pos[id].x + NW;
    return y;
  }
  const rts = D.nodes.filter(n => !parents[n.id] && !isIso(n));
  rts.sort((a, b) => (a.src ? 0 : 1) - (b.src ? 0 : 1)
    || ((children[a.id] || []).length ? 0 : 1) - ((children[b.id] || []).length ? 0 : 1));
  const PERCOL = 5;
  function leaves(id, vis) {
    vis = vis || {};
    const ch = (children[id] || []).filter(c => id2n[c] && !vis[c]);
    if (!ch.length) return 1;
    vis[id] = 1;
    let s = 0;
    ch.forEach(c => { s += leaves(c, vis); });
    return s;
  }
  let s0 = 0, cnt0 = 0;
  while (s0 < rts.length) {
    let e0 = s0; cnt0 = 0;
    while (e0 < rts.length) {
      const lv = leaves(rts[e0].id);
      if (cnt0 > 0 && cnt0 + lv > 13) break;
      cnt0 += lv; e0++;
    }
    leafIdx = 0; gMaxX = colOff;
    rts.slice(s0, e0).forEach(r => place(r.id, 0));
    colOff = gMaxX + 46;
    s0 = e0;
  }
  let maxY = 0;
  Object.values(pos).forEach(p => { if (p.y > maxY) maxY = p.y; });
  const isoList = D.nodes.filter(isIso);
  let treeMaxX = 0;
  Object.values(pos).forEach(p => { if (p.x + NW > treeMaxX) treeMaxX = p.x + NW; });
  const PER = 2;
  isoList.forEach((n, i) => {
    pos[n.id] = { x: treeMaxX + 60 + (i % PER) * COLX, y: Math.floor(i / PER) * ROWY };
  });
  D.nodes.forEach(n => { if (!pos[n.id]) { pos[n.id] = { x: 0, y: (leafIdx++) * ROWY }; } });
}

function fit(force) {
  const xs = Object.values(pos).map(p => p.x), ys = Object.values(pos).map(p => p.y);
  const minX = Math.min(...xs) - 110, maxX = Math.max(...xs) + NW + 110;
  const minY = Math.min(...ys) - 90, maxY = Math.max(...ys) + NH + 90;
  const kFit = Math.min(innerWidth / (maxX - minX), innerHeight / (maxY - minY)) * 0.96;
  const k = force ? kFit : Math.max(0.52, kFit);
  view = { k, x: (innerWidth - (maxX - minX) * k) / 2 - minX * k, y: 58 - minY * k };
  draw();
}

function draw() {
  g.setTransform(1, 0, 0, 1, 0, 0);
  g.clearRect(0, 0, cv.width, cv.height);
  g.fillStyle = '#06080a'; g.fillRect(0, 0, cv.width, cv.height);
  g.save(); g.translate(view.x, view.y); g.scale(view.k, view.k);

  D.links.forEach(l => {
    const a = pos[l.f], b = pos[l.t];
    if (!a || !b) return;
    const hot = sel && (l.f === sel || l.t === sel);
    const ax = a.x + NW, ay = a.y + NH / 2, bx = b.x, by = b.y + NH / 2;
    g.beginPath();
    g.moveTo(ax, ay);
    g.bezierCurveTo(ax + 52, ay, bx - 52, by, bx, by);
    g.strokeStyle = hot ? '#ffa94d' : '#2f7ea3';
    g.lineWidth = hot ? 2.2 : 1.2;
    g.stroke();
    g.beginPath();
    g.moveTo(bx, by); g.lineTo(bx - 7, by - 5); g.lineTo(bx - 7, by + 5);
    g.fillStyle = hot ? '#ffa94d' : '#2f7ea3'; g.fill();
    if (l.v) {
      g.fillStyle = hot ? '#ffd9a8' : '#7fc4e8'; g.font = '10px Arial'; g.textAlign = 'center';
      g.fillText(l.v, (ax + bx) / 2, (ay + by) / 2 - 5);
    }
  });

  D.nodes.forEach(n => {
    const p = pos[n.id]; if (!p) return;
    const hasIn = n.inl > 0 || !!parents[n.id], hot = sel === n.id;
    g.beginPath();
    if (g.roundRect) g.roundRect(p.x, p.y, NW, NH, 5); else g.rect(p.x, p.y, NW, NH);
    g.fillStyle = hot ? '#14496b' : (hasIn ? '#0d3550' : '#3a2308'); g.fill();
    g.strokeStyle = hot ? '#ffd166' : (hasIn ? '#4fb8f0' : '#ff9a3c');
    g.lineWidth = hot ? 2.4 : 1.4; g.stroke();
    g.fillStyle = '#ffffff'; g.font = 'bold 13px "Segoe UI",Arial'; g.textAlign = 'center';
    g.fillText(n.id, p.x + NW / 2, p.y + 22);
    g.fillStyle = hasIn ? '#8fd8ff' : '#ffc98a'; g.font = '10px Arial';
    g.fillText('Pe ' + n.pe + ' · ' + n.n + ' 回路', p.x + NW / 2, p.y + 42);
  });
  g.restore();
}

function resize() {
  const dpr = devicePixelRatio || 1;
  cv.width = innerWidth * dpr; cv.height = innerHeight * dpr;
  cv.style.width = innerWidth + 'px'; cv.style.height = innerHeight + 'px';
  g.setTransform(1, 0, 0, 1, 0, 0); g.scale(dpr, dpr);
  fit();
}

let drag = null;
cv.addEventListener('mousedown', e => drag = { x: e.clientX, y: e.clientY, vx: view.x, vy: view.y, moved: false });
addEventListener('mousemove', e => {
  if (!drag) return;
  const dx = e.clientX - drag.x, dy = e.clientY - drag.y;
  if (Math.abs(dx) + Math.abs(dy) > 2) drag.moved = true;
  view.x = drag.vx + dx; view.y = drag.vy + dy; draw();
});
addEventListener('mouseup', e => {
  if (drag && !drag.moved) {
    const mx = (e.clientX - view.x) / view.k, my = (e.clientY - view.y) / view.k;
    let hit = null;
    Object.keys(pos).forEach(id => {
      const p = pos[id];
      if (mx >= p.x && mx <= p.x + NW && my >= p.y && my <= p.y + NH) hit = id;
    });
    sel = (hit === sel) ? null : hit;
    draw(); updHud();
  }
  drag = null;
});
cv.addEventListener('wheel', e => {
  e.preventDefault();
  const k = Math.max(0.15, Math.min(3, view.k * (e.deltaY < 0 ? 1.12 : 0.89)));
  const mx = (e.clientX - view.x) / view.k, my = (e.clientY - view.y) / view.k;
  view.x = e.clientX - mx * k; view.y = e.clientY - my * k; view.k = k;
  draw();
}, { passive: false });

function updHud() {
  if (!sel) { hud.innerHTML = '<b>' + D.nodes.length + '</b> 柜 · 边 <b>' + D.links.length + '</b> · 点柜看上下游'; return; }
  hud.innerHTML = '选中 <b>' + sel + '</b> · 上游 <b>' + ((parents[sel] || []).join('、') || '—') +
    '</b> · 下游 <b>' + ((children[sel] || []).join('、') || '—') + '</b>';
}

document.getElementById('bFit').onclick = () => fit(true);
document.getElementById('bIso').onclick = () => {
  showIso = !showIso;
  document.getElementById('bIso').classList.toggle('on', !!showIso);
  location.reload();
};

layout(); resize(); updHud();
</script>
</body>
</html>'''

out = r'G:\配电规划设计\web\topology.html'
open(out, 'w', encoding='utf-8').write(HTML.replace('__DATA__', DATA))
print('写出', out, '节点', len(nodes), '边', len(links), '虚节点', len(orphan) if False else '')
