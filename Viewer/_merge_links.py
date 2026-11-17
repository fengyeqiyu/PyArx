# -*- coding: utf-8 -*-
"""把工艺配电图纸（GYAP/AGVAP）的跨图出线并入 101# 系统图连接字典。"""
import json

A = json.load(open(r'G:\配电规划设计\web\101\el_links.json', encoding='utf-8'))
B = json.load(open(r'G:\PyArx\viewer\export\gy_links.json', encoding='utf-8'))

nodes = {n['id']: n for n in A['nodes']}
for n in B['nodes']:
    if n['id'] not in nodes:
        nodes[n['id']] = {'id': n['id'], 'pe': n['pe'], 'n': n['n'], 'inl': 1}

links = list(A['links'])
seen = {(l.get('from'), l.get('to')) for l in links}
for l in B['links']:
    k = (l['from'], l['to'])
    if k not in seen:
        seen.add(k)
        links.append({'from': l['from'], 'to': l['to'], 'via': l.get('via', ''),
                      'cable': l.get('cable', '')})

out = {'nodes': list(nodes.values()), 'links': links}
json.dump(out, open(r'G:\PyArx\viewer\export\el_links.json', 'w', encoding='utf-8'),
          ensure_ascii=False, indent=1)
print('合并节点', len(out['nodes']), '边', len(links))
