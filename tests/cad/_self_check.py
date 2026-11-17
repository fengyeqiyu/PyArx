# -*- coding: utf-8 -*-
"""DXF 真值产物自查（临时脚本，仅标准库）。"""
import json, random, re, sys

def load_pairs(path):
    raw = open(path, 'rb').read()
    if raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]
    try:
        t = raw.decode('utf-8')
    except UnicodeDecodeError:
        t = raw.decode('gbk', errors='replace')
    lines = t.splitlines()
    pairs = []
    i, n = 0, len(lines)
    while i + 1 < n:
        cr = lines[i].strip()
        if cr == '':
            i += 1; continue
        try:
            code = int(cr)
        except ValueError:
            i += 1; continue
        pairs.append((code, lines[i+1].rstrip('\r')))
        i += 2
    return pairs

def analyze(dxf):
    pairs = load_pairs(dxf)
    section = None; in_table = None
    layers = set(); blocks = set()
    ent_handles = []; ent_types = []
    cur = None  # current entity pair dict
    def flush():
        nonlocal cur
        if cur is not None:
            ent_handles.append(cur.get(5))
            ent_types.append(cur.get('_type'))
        cur = None

    j = 0
    while j < len(pairs):
        code, val = pairs[j]
        if code == 0 and val == 'SECTION':
            flush(); section = None; j += 1; continue
        if code == 2 and section is None:
            section = val; j += 1; continue
        if section == 'TABLES':
            if code == 0 and val == 'TABLE':
                in_table = None; j += 1; continue
            if code == 2 and in_table is None:
                in_table = val; j += 1; continue
            if code == 0 and val == 'ENDTAB':
                in_table = None; j += 1; continue
            if code == 0:
                j += 1; continue
            if in_table == 'LAYER' and code == 2:
                layers.add(val)
            if in_table == 'BLOCK_RECORD' and code == 2:
                blocks.add(val)
            j += 1; continue
        if section in ('ENTITIES', 'BLOCKS'):
            if code == 0:
                flush()
                if val in ('BLOCK', 'ENDBLK', 'SEQEND', 'ENDSEC', 'EOF', 'SECTION'):
                    j += 1; continue
                cur = {'_type': val}; j += 1; continue
            if cur is not None:
                cur.setdefault(code, []).append(val)
            j += 1; continue
        j += 1
    flush()
    return {'layers': layers, 'blocks': blocks, 'handles': ent_handles, 'types': ent_types}

def check(dxf, jsonp, seed):
    d = json.load(open(jsonp, encoding='utf-8'))
    raw = open(jsonp, 'rb').read()
    bom = raw[:3] == b'\xef\xbb\xbf'
    a = analyze(dxf)
    s = d['summary']
    problems = []

    if bom: problems.append('JSON has UTF-8 BOM')
    if s['model_space_entities'] + s['block_internal_entities'] != len(d['entities']):
        problems.append('entity list length mismatch')
    if s['model_space_entities'] != s['type_counts_model_space'] and False:
        pass
    # handle unique
    handles = [e['handle'] for e in d['entities'] if e['handle']]
    if len(handles) != len(set(handles)):
        problems.append('duplicate handles in JSON')
    if len(handles) != len(d['entities']):
        problems.append('missing handle on %d entities' % (len(d['entities']) - len(handles)))
    # layer / block sets
    jlayers = {x['name'] for x in d['layers']}
    jblocks = {x['name'] for x in d['blocks']}
    if jlayers != a['layers']:
        problems.append('layer set mismatch: only DXF=%s only JSON=%s' % (a['layers']-jlayers, jlayers-a['layers']))
    if jblocks != a['blocks']:
        problems.append('block set mismatch: only DXF=%s only JSON=%s' % (a['blocks']-jblocks, jblocks-a['blocks']))
    # raw entity count vs JSON
    if len(a['handles']) != len(d['entities']):
        problems.append('raw entity count %d != JSON %d' % (len(a['handles']), len(d['entities'])))

    # random 3 coordinate cross-check
    rng = random.Random(seed)
    sample = rng.sample(d['entities'], min(3, len(d['entities'])))
    rawpairs = load_pairs(dxf)
    # index entity blocks by handle
    ent = {}
    cur = None
    for code, val in rawpairs:
        if code == 0:
            cur = None
            if val not in ('BLOCK','ENDBLK','SEQEND','ENDSEC','EOF','SECTION'):
                cur = {'_type': val}
            continue
        if cur is not None:
            cur.setdefault(code, []).append(val)
            if code == 5:
                ent[val] = cur
    coord_codes = [10,20,30,11,21,31,12,22,40,50,51,41,42]
    checked = 0
    for e in sample:
        raw = ent.get(e['handle'])
        if raw is None:
            problems.append('handle %s not found raw' % e['handle']); continue
        g = e['geometry']
        # compare common point codes present in raw
        for code in coord_codes:
            if code in raw:
                rv = float(raw[code][-1])
                jv = None
                # map code into geometry (reuse known layouts)
                def getpt(keys):
                    for k in keys:
                        if g.get(k) is not None:
                            return g[k]
                if code in (10,): jv = (g.get('center') or g.get('position') or g.get('insertion') or g.get('def_point') or g.get('location') or g.get('point') or [None]*3)[0]
                elif code in (20,): jv = (g.get('center') or g.get('position') or g.get('insertion') or g.get('def_point') or g.get('location') or g.get('point') or [None]*3)[1]
                elif code in (30,): jv = (g.get('center') or g.get('position') or g.get('insertion') or g.get('def_point') or g.get('location') or g.get('point') or [None]*3)[2]
                elif code in (11,): jv = (g.get('end') or g.get('text_middle') or g.get('major_axis') or [None]*3)[0]
                elif code in (21,): jv = (g.get('end') or g.get('text_middle') or g.get('major_axis') or [None]*3)[1]
                elif code in (31,): jv = (g.get('end') or g.get('text_middle') or g.get('major_axis') or [None]*3)[2]
                elif code == 40: jv = g.get('radius') or g.get('height')
                elif code == 50: jv = g.get('start_angle') or g.get('rotation')
                elif code == 51: jv = g.get('end_angle')
                elif code == 41: jv = g.get('scale_x')
                elif code == 42: jv = g.get('scale_y')
                if jv is not None and abs(jv - rv) > 1e-6:
                    problems.append('coord mismatch h=%s code=%d json=%r raw=%r' % (e['handle'], code, jv, rv))
                checked += 1
    print('=== %s' % jsonp.split('\\')[-1])
    print(' entities JSON=%d raw=%d  layers=%d blocks=%d  sampled=%d coords checked'
          % (len(d['entities']), len(a['handles']), len(jlayers), len(jblocks), checked))
    print(' model types:', s['type_counts_model_space'])
    print(' block types:', s['type_counts_in_blocks'])
    if problems:
        print(' PROBLEMS:')
        for p in problems: print('  -', p)
        return 1
    print(' OK')
    return 0

rc = 0
rc |= check(r'G:\PyArx\tests\dwg\楼层平面-10KV电缆桥架.dxf',
            r'G:\PyArx\tests\dwg\truth_楼层平面-10KV电缆桥架.json', 7)
rc |= check(r'G:\PyArx\tests\dwg\楼层平面-强电.dxf',
            r'G:\PyArx\tests\dwg\truth_楼层平面-强电.json', 13)
sys.exit(rc)
