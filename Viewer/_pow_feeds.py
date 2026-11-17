# -*- coding: utf-8 -*-
"""提取供电图里的馈电清单（柜名 + 功率），并按列归到对应母线/变压器。"""
import json
import re

d = json.load(open(r'G:\PyArx\viewer\web\scene_pow.json', encoding='utf-8'))
tx = [(t[0], t[1], str(t[6]).strip()) for t in (d.get('tx') or [])]
FEED = re.compile(r'^(101-1-1\S+)\s+([\d.]+)\s*KW', re.I)
feeds = [(x, y, m.group(1), float(m.group(2))) for (x, y, s) in tx
         for m in [FEED.match(s)] if m]
print('馈电条目', len(feeds))
cols = {}
for x, y, nm, kw in feeds:
    cols.setdefault(round(x / 600), []).append((y, x, nm, kw))
for k in sorted(cols):
    it = sorted(cols[k])
    xs = [v[1] for v in it]
    print('列 x≈%.0f 条数%d' % (sum(xs) / len(xs), len(it)))
    for y, x, nm, kw in it:
        print('   ', round(y), nm, kw)
