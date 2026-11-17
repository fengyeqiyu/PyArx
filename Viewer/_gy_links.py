# -*- coding: utf-8 -*-
"""从工艺配电图纸（GYAP 系列）提取对 AL/FFU/FQAP 柜的出线，形成跨图连接。"""
import json
import re

SC = r'G:\PyArx\viewer\web\scene_gy.json'
CABS = r'G:\PyArx\viewer\export\cabinets_all.json'
OUT = r'G:\PyArx\viewer\export\gy_links.json'

d = json.load(open(SC, encoding='utf-8'))
cabs = json.load(open(CABS, encoding='utf-8'))['cabinets']
boxes = [(c['name'], c['box']) for c in cabs if c.get('box')]
texts = [(t[0], t[1], str(t[6]).strip()) for t in d.get('tx') or []]
LOOP = re.compile(r'^W[A-Z]{0,3}\d+$')
CABLE = re.compile(r'YJV|BV|ZR-|WDZ|KVV|YJV22')
NAMES = ['AL%d' % i for i in range(11, 30)] + ['FFU1', 'FQAP3', 'FQAP4', 'FQAP5']

links, seen = [], set()
for x, y, s in texts:
    tgt = None
    for nm in NAMES:
        if s == nm or s == '101-1-1' + nm or s.endswith('-' + nm):
            tgt = nm
            break
    if not tgt:
        continue
    host = [nm for nm, b in boxes if b[0] <= x <= b[2] and b[1] <= y <= b[3]]
    if not host:
        continue
    host = host[0]
    near = sorted((x2, s2) for (x2, y2, s2) in texts if abs(y2 - y) < 700 and x2 > x)
    loop = next((s2 for (x2, s2) in near if LOOP.match(s2)), '')
    cable = next((s2 for (x2, s2) in near if CABLE.search(s2)), '')
    key = (host, tgt)
    if key in seen:
        continue
    seen.add(key)
    links.append({'from': host, 'to': '101-1-1' + tgt, 'via': loop, 'cable': cable})

print('补链接', len(links))
for l in links:
    print(' ', l['from'], '->', l['to'], l['via'], l['cable'])

nodes = [{'id': c['name'], 'pe': float(c.get('pe') or 0),
          'n': len(c.get('loops') or []), 'inl': 0} for c in cabs]
json.dump({'nodes': nodes, 'links': links}, open(OUT, 'w', encoding='utf-8'),
          ensure_ascii=False, indent=1)
print('写出', OUT)
