# -*- coding: utf-8 -*-
"""把供电（母线）图的馈电清单并入连接字典：
   1) 21 条“变电所引来”文字源改写为 0.4kV 母线根；
   2) 补齐清单中尚无上游的柜（FQAP3/4/5、FQAP1、AT1/AT2）。"""
import json

L = json.load(open(r'G:\PyArx\viewer\export\el_links.json', encoding='utf-8'))
M3 = '101#变电所3-0.4kV母线'
M4 = '101#变电所4-0.4kV母线'

FEEDS = [
    ('101-1-1FFUAP1', 164.0, M3), ('101-1-1HEFAP1', 110.0, M3), ('101-1-1KTAP3', 94.0, M3),
    ('101-1-1KTAP2', 77.0, M3), ('101-1-1RCUAP2', 107.0, M3), ('101-1-1FQAP2', 90.0, M3),
    ('101-1-1FQAP1', 56.0, M3), ('101-1-1FJAP1', 112.0, M3), ('101-1-1KTAP1', 113.0, M3),
    ('101-1-1RCUAP1', 226.4, M3), ('101-1-1ZAL1', 212.0, M3), ('101-1-1AT1', 38.0, M3),
    ('101-1-1FQAP5', 150.0, M4), ('101-1-1KTAP4', 106.0, M4), ('101-1-1FQAP3', 110.0, M4),
    ('101-1-1ZAL2', 198.0, M4), ('101-1-1AT2', 30.0, M4), ('101-1-1KTAP9', 231.5, M4),
    ('101-1-1KTAP8', 205.0, M4), ('101-1-1KTAP7', 261.0, M4), ('101-1-1KTAP6', 96.0, M4),
    ('101-1-1KTAP5', 37.0, M4), ('101-1-1FQAP4', 150.0, M4), ('101-1-1ITAP1', 24.0, M4),
    ('101-1-1FFUAP2', 171.0, M4), ('101-1-1HEFAP2', 74.0, M4), ('101-1-1RCUAP3', 47.5, M4),
]
BUS = {nm: bus for nm, kw, bus in FEEDS}
KW = {nm: kw for nm, kw, bus in FEEDS}

# 1) 文字源 -> 母线
for l in L['links']:
    if l['from'] in ('变电所引来', '中试车间变电所引来', '由中试车间变电所引来',
                     '废气电源柜2 由变电所直供') or '变电所' in l['from']:
        bus = BUS.get(l['to'])
        if bus:
            l['from'] = bus
            l['via'] = (l.get('via') or '') + (' ' if l.get('via') else '') + '馈电'

# 2) 母线节点
ids = {n['id'] for n in L['nodes']}
for m in (M3, M4):
    if m not in ids:
        L['nodes'].append({'id': m, 'pe': 0, 'n': 0, 'inl': 1, 'src': 1})

# 3) 补齐无上游的柜
par = {l['to'] for l in L['links']}
seen = {(l['from'], l['to']) for l in L['links']}
added = []
for nm, kw, bus in FEEDS:
    if nm not in ids:
        L['nodes'].append({'id': nm, 'pe': kw, 'n': 0, 'inl': 1})
        ids.add(nm)
    if nm in par or (bus, nm) in seen:
        continue
    L['links'].append({'from': bus, 'to': nm, 'via': '馈电 %.1fkW' % kw, 'cable': ''})
    seen.add((bus, nm))
    added.append((bus, nm, kw))

json.dump(L, open(r'G:\PyArx\viewer\export\el_links.json', 'w', encoding='utf-8'),
          ensure_ascii=False, indent=1)
no_parent = [n['id'] for n in L['nodes'] if n['id'] not in par and n.get('src') != 1]
print('节点', len(L['nodes']), '边', len(L['links']))
print('新增馈电边', len(added))
for a in added:
    print('  ', a)
print('仍无上游', len(no_parent), no_parent)
