# -*- coding: utf-8 -*-
"""自动纠错：柜名以框内实际柜号为准；无柜号时取柜体标注；并标记纠正/重名。"""
import json, re
from collections import Counter

SC = r'G:\PyArx\viewer\web\scene.json'
CA = r'G:\PyArx\viewer\export\cabinets_all.json'

d = json.load(open(SC, encoding='utf-8'))
C = json.load(open(CA, encoding='utf-8'))
tx = d['tx']
pat = re.compile(r'101-1-1[A-Z0-9]+$')

changes = []
for o in C['cabinets']:
    b = o['box']
    inner = [t for t in tx if b[0] <= t[0] <= b[2] and b[1] <= t[1] <= b[3] and t[1] >= 500000]
    ids = []
    for t in inner:
        s = str(t[6])
        if pat.match(s) and s not in ids:
            ids.append(s)
    anns = []
    for t in inner:
        s = str(t[6])
        if ('配电柜' in s or '配电箱' in s) and len(s) < 12 and s not in anns:
            anns.append(s)

    old = o.get('name') or ''
    if ids:
        if old in ids:
            new = old
        elif len(ids) == 1:
            new = ids[0]
        else:
            cx, cy = (b[0] + b[2]) / 2, (b[1] + b[3]) / 2
            cand = [((t[0] - cx) ** 2 + (t[1] - cy) ** 2, str(t[6])) for t in inner if pat.match(str(t[6]))]
            new = min(cand)[1]
    else:
        new = anns[0] if anns else old

    if new and new != old:
        o['name'] = new
        o['w'] = ((o.get('w') or '') + (' ' if o.get('w') else '') + '柜名纠正').strip()
        changes.append((o['i'] + 1, old, new))

cnt = Counter(o.get('name') for o in C['cabinets'])
for o in C['cabinets']:
    if cnt[o.get('name')] > 1:
        o['w'] = ((o.get('w') or '') + (' ' if o.get('w') else '') + '重名').strip()

json.dump(C, open(CA, 'w', encoding='utf-8'), ensure_ascii=False)
print('纠正台数', len(changes))
for i, a, b2 in changes:
    print(i, a, '->', b2)
print('重名柜', [k for k, v in cnt.items() if v > 1])
