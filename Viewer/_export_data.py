# -*- coding: utf-8 -*-
"""汇总三张图纸的识别结果，输出统一拓扑数据（节点 + 边 + 来源 + 功率）。"""
import json
import os

L = json.load(open(r'G:\PyArx\viewer\export\el_links.json', encoding='utf-8'))
OUT = r'G:\配电规划设计\data'
os.makedirs(OUT, exist_ok=True)

def src_of(nid):
    if '母线' in nid:
        return 'pow'
    if 'GYAP' in nid or 'AGVAP' in nid:
        return 'gy'
    return '101'

par, chi = {}, {}
for l in L['links']:
    f, t = l.get('from'), l.get('to')
    if not f or not t:
        continue
    par.setdefault(t, []).append(f)
    chi.setdefault(f, []).append(t)

nodes = []
for n in L['nodes']:
    nid = n['id']
    nodes.append({
        'id': nid,
        'source': src_of(nid),
        'pe': n.get('pe') or 0,
        'loops': n.get('n') or 0,
        'hasIn': (n.get('inl') or 0) > 0 or nid in par,
        'parent': par.get(nid, []),
        'children': chi.get(nid, []),
        'root': nid not in par,
        'island': nid not in par and nid not in chi,
    })

links = [{'from': l['from'], 'to': l['to'], 'via': l.get('via', ''), 'cable': l.get('cable', '')}
         for l in L['links'] if l.get('from') and l.get('to')]
out = {
    'nodes': nodes,
    'links': links,
    'stat': {
        'nodes': len(nodes), 'links': len(links),
        'source_count': {s: sum(1 for n in nodes if n['source'] == s) for s in ('pow', '101', 'gy')},
        'roots': sum(1 for n in nodes if n['root']),
        'islands': sum(1 for n in nodes if n['island']),
    }
}
json.dump(out, open(os.path.join(OUT, 'topology.json'), 'w', encoding='utf-8'),
          ensure_ascii=False, indent=1)
print(json.dumps(out['stat'], ensure_ascii=False))
print('孤岛:', [n['id'] for n in nodes if n['island']])
print('根:', [n['id'] for n in nodes if n['root']][:12])
