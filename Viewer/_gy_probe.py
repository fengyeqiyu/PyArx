# -*- coding: utf-8 -*-
import json
SC = r'G:\PyArx\viewer\web\scene_gy.json'
CAB = r'G:\PyArx\viewer\export\cabinets_all.json'
d = json.load(open(SC, encoding='utf-8'))
cabs = json.load(open(CAB, encoding='utf-8'))['cabinets']
boxes = []
for c in cabs:
    b = c.get('box')
    if b:
        boxes.append((c.get('name'), b))
print('柜框', len(boxes))
for nm, b in boxes[:3]:
    print(nm, b)
texts = [(t[0], t[1], str(t[6]).strip()) for t in d.get('tx') or []]
for name in ['AL11', 'AL12', 'FFU1', 'FQAP3', 'AL26']:
    hits = [(x, y, s) for (x, y, s) in texts if s == name or s.endswith(name)]
    print('---', name, len(hits))
    for x, y, s in hits[:3]:
        box = [nm for nm, b in boxes if b[0] <= x <= b[2] and b[1] <= y <= b[3]]
        near = [(x2, y2, s2) for (x2, y2, s2) in texts if abs(y2 - y) < 600 and x2 > x]
        print(' ', s, x, y, '框', box[:2], '右侧', [s2 for (x2, y2, s2) in near[:6]])
