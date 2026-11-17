# -*- coding: utf-8 -*-
"""把每台柜识别为 ElNode：进线 InLine(n)、出线 WP/WL、备用 RS，含开关/电缆/去向/功率。"""
import json, re

SC = r'G:\PyArx\viewer\web\scene.json'
CA = r'G:\PyArx\viewer\export\cabinets_all.json'
OUT = r'G:\PyArx\viewer\export\el_nodes.json'

d = json.load(open(SC, encoding='utf-8'))
C = json.load(open(CA, encoding='utf-8'))['cabinets']
tx = d['tx']

re_loop = re.compile(r'^W[A-Z]{0,3}\d+$')
re_pow = re.compile(r'\d+(?:\.\d+)?\s*[Kk]W')
re_brk = re.compile(r'MCB|MCCB|RCBO|NA\d')


def S(t):
    return str(t[6]).strip()


def nearest(lst, y, tol):
    best, bd = None, tol
    for t in lst:
        dd = abs(t[1] - y)
        if dd <= bd:
            bd, best = dd, t
    return best


def parse(o):
    b = o['box']; nm = o.get('name') or ''
    inner = [t for t in tx if b[0] <= t[0] <= b[2] and b[1] <= t[1] <= b[3] and t[1] >= 500000]
    LOOP = []
    seen_loop = set()
    for t in sorted([t for t in inner if re_loop.match(S(t))], key=lambda t: -t[1]):
        if S(t) not in seen_loop:
            seen_loop.add(S(t))
            LOOP.append(t)
    BRK = [t for t in inner if re_brk.search(S(t)) and not re_pow.search(S(t))]
    CAB = [t for t in inner if ('YJV' in S(t) or 'BV' in S(t))]
    TGT = [t for t in inner if ('101-1' in S(t)) and 'SUB' not in S(t) and S(t) != nm]
    POW = [t for t in inner if re_pow.search(S(t))]

    def tgt_for(w):
        # 去向：同回路行、在编号右侧、x 最大的非器件文字（设备名/柜号/用途）
        cand = []
        for t in inner:
            if abs(t[1] - w[1]) <= 500 and t[0] > w[0]:
                s = S(t)
                if re_loop.match(s):
                    continue
                if re.search(r'MCB|RCBO|NA\d|YJV|BV|JDG|SC\d|mA|mm', s):
                    continue
                if re.fullmatch(r'\d+(?:\.\d+)?[Kk]W', s):
                    continue
                if re.match(r'^L\d$', s) or s in ('备用', '注') or '分箱' in s:
                    continue
                cand.append(t)
        return max(cand, key=lambda t: t[0]) if cand else None

    out = []
    for w in sorted(LOOP, key=lambda t: -t[1]):
        tt = tgt_for(w)
        out.append({
            'id': S(w),
            'breaker': (S(nearest(BRK, w[1], 600)) if nearest(BRK, w[1], 600) else ''),
            'cable': (S(nearest(CAB, w[1], 600)) if nearest(CAB, w[1], 600) else ''),
            'target': (re.sub(r'\s*\d+(?:\.\d+)?[Kk]W', '', S(tt)).strip() if tt else ''),
            'power': (S(nearest(POW, w[1], 900)) if nearest(POW, w[1], 900) else ''),
        })

    # 展开 "WL3~16同上" / "WC2~WC19同上" 这类范围省略（本图纸固定写法）
    exp_ids = []
    have = [o['id'] for o in out]
    for t in inner:
        m = re.match(r'^W([A-Z]{0,3})(\d+)~(?:W[A-Z]{0,3})?(\d+)同上', S(t))
        if m:
            p, a, b = m.group(1), int(m.group(2)), int(m.group(3))
            for n in range(a, b + 1):
                eid = 'W' + p + str(n)
                if eid not in have and (eid, p, n) not in exp_ids:
                    exp_ids.append((eid, p, n))
    base = {o['id']: o for o in out}
    for eid, p, n in exp_ids:
        ref = None
        for k in range(n - 1, 0, -1):
            if 'W' + p + str(k) in base:
                ref = base['W' + p + str(k)]
                break
        if ref:
            out.append({'id': eid, 'breaker': ref['breaker'], 'cable': ref['cable'],
                        'target': ref['target'], 'power': ref['power']})

    inl = []
    for t in sorted([t for t in inner if '进线' in S(t)], key=lambda t: -t[1]):
        inl.append({'id': nm + '-InLine%d' % (len(inl) + 1),
                    'cable': re.sub(r'^进线\s*', '', S(t)),
                    'breaker': '', 'from': ''})
    mccb = [t for t in inner if re.match(r'MCCB\s*[56]\d\d', S(t))]
    if inl and mccb:
        inl[0]['breaker'] = S(mccb[0])
    src = [t for t in inner if '变电所' in S(t)]
    if inl and src:
        inl[0]['from'] = S(src[0])

    spr = [{'id': 'RS%d' % (i + 1), 'note': '备用'} for i, t in enumerate([t for t in inner if '备用' in S(t)])]

    # 控制回路（动力柜风机控制）：图上标 "二次线路G/K…"，统一编码 ctrl-num
    ctrl = []
    seen_ctrl = set()
    for t in sorted([t for t in inner if re.search(r'二次线路[A-Za-z]', S(t))], key=lambda t: -t[1]):
        s = S(t)
        m = re.search(r'二次线路[A-Za-z]', s)
        key = m.group(0) if m else s
        if key in seen_ctrl:
            continue
        seen_ctrl.add(key)
        cable = s.replace(m.group(0), '').strip() if m else s
        ctrl.append({'id': 'ctrl-%d' % (len(ctrl) + 1), 'name': (m.group(0) if m else ''), 'cable': cable})

    return {'id': nm, 'type': 'ElNode', 'box': b,
            'inLines': inl, 'outLines': out, 'controls': ctrl, 'spares': spr,
            'pe': o.get('pe'), 'ijs': o.get('ijs')}


nodes = [parse(o) for o in C]

# —— 柜间连接：某柜没标进线，但上游柜的出线 target 指向它，就用上游出线补全进线 ——
def norm(s):
    return s.replace('101-1-1', '').replace('101-1', '').strip()

cab_ids = {norm(n['id']): n['id'] for n in nodes if n['id']}
up = {}
for n in nodes:
    for o in n['outLines']:
        t = o['target']
        if t:
            key = norm(t)
            if key in cab_ids:
                up.setdefault(key, []).append({'from': n['id'], 'loop': o['id'], 'cable': o['cable'], 'power': o['power']})

links = []
missing_before = sum(1 for n in nodes if not n['inLines'])
for n in nodes:
    if not n['inLines']:
        key = norm(n['id'])
        if key in up:
            n['inLines'] = [{'id': '%s-InLine%d' % (n['id'], i + 1), 'from': x['from'], 'via': x['loop'],
                             'cable': x['cable'], 'breaker': ''} for i, x in enumerate(up[key])]
    for il in n['inLines']:
        links.append({'to': n['id'], 'from': il.get('from', ''), 'via': il.get('via', ''), 'cable': il.get('cable', '')})

json.dump({'nodes': nodes, 'links': links, 'upstream': up},
          open(r'G:\PyArx\viewer\export\el_links.json', 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
json.dump({'nodes': nodes}, open(OUT, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print('写出', OUT, len(nodes), '节点；连接', len(links), '条；原无进线', missing_before, '台，补全后无进线',
      sum(1 for n in nodes if not n['inLines']), '台')
