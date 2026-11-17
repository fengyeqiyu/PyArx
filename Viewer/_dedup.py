# -*- coding: utf-8 -*-
"""自己判断：1) #1 由 W 系列照明回路判为 ZAL1；2) 删掉 FJAP1 的 0 回路误框；重排 i。"""
import json

CA = r'G:\PyArx\viewer\export\cabinets_all.json'
C = json.load(open(CA, encoding='utf-8'))
cabs = C['cabinets']
log = []

for o in cabs:
    if o['name'] == '101-1-1KTAP7':
        loops = o.get('loops') or []
        if loops and all(re_ok for re_ok in [str(x).startswith('W') and not str(x).startswith('WP') for x in loops]):
            if not any(c2['name'] == '101-1-1ZAL1' for c2 in cabs if c2 is not o):
                o['name'] = '101-1-1ZAL1'
                o['w'] = ((o.get('w') or '') + ' 由照明W回路判为ZAL1').strip()
                log.append((o['i'] + 1, 'KTAP7', 'ZAL1'))

dropped = []
kept = []
for o in cabs:
    if o['name'] == '101-1-1FJAP1' and not (o.get('loops') or []):
        dropped.append(o['i'] + 1)
    else:
        kept.append(o)
C['cabinets'] = kept
for n, o in enumerate(C['cabinets']):
    o['i'] = n

json.dump(C, open(CA, 'w', encoding='utf-8'), ensure_ascii=False)
print('改名', log)
print('删除误框台', dropped, ' 剩余', len(C['cabinets']))
