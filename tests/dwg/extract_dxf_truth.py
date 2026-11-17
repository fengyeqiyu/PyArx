# -*- coding: utf-8 -*-
"""
extract_dxf_truth.py —— 纯标准库 DXF 真值抽取器（验收对拍用）。

用法：
    python extract_dxf_truth.py <input.dxf> <output.truth.json>

解析规则：
  * 按 group code / value 成对读取；输出每个实体的 handle(组码 5)、type、layer、
    color(62)、ltype(6) 与关键几何字段；坐标以双精度 float 写入，不做四舍五入。
  * 中文等非 ASCII 名称/文字：还原 DXF 中的 \\U+XXXX（UTF-16 码元）转义；
    JSON 以 UTF-8 无 BOM 写出。
  * 模型空间实体来自 ENTITIES 段；块内部实体在 BLOCKS 段单独计数并标明所属块。

仅使用 Python 标准库。
"""
import json
import re
import sys

U_RE = re.compile(r'\\U\+([0-9A-Fa-f]{4})')


def decode_unicode(s):
    """还原 DXF \\U+XXXX 转义（每个码元一个 UTF-16 BMP 字符）。"""
    if s is None:
        return None
    def repl(m):
        return chr(int(m.group(1), 16))
    return U_RE.sub(repl, s)


def as_float(v):
    if v is None or v == '':
        return None
    try:
        return float(v)
    except ValueError:
        return None


def as_int(v):
    if v is None or v == '':
        return None
    try:
        return int(v.strip())
    except ValueError:
        try:
            return int(float(v))
        except ValueError:
            return None


def read_pairs(lines):
    """迭代 (code:int, value:str)。DXF 文本按行成对：code 行、value 行。"""
    i = 0
    n = len(lines)
    while i + 1 < n:
        code_raw = lines[i].strip()
        if code_raw == '':
            i += 1
            continue
        try:
            code = int(code_raw)
        except ValueError:
            i += 1
            continue
        value = lines[i + 1].rstrip('\r')
        yield code, value
        i += 2


def parse_geometry(etype, pairs):
    """按实体类型抽取关键几何字段；pairs 为该实体的 {code: [values...]}（组码重复时保序）。"""
    g = {}

    def last(code):
        vals = pairs.get(code)
        return vals[-1] if vals else None

    def allvals(code):
        return pairs.get(code, [])

    def full_text():
        """DXF 长文本：前面的组码 3 为续行，最后一段在组码 1。"""
        return decode_unicode(''.join(allvals(3)) + (last(1) or ''))

    def pt(xc, yc, zc=30):
        return [as_float(last(xc)), as_float(last(yc)), as_float(last(zc))]

    if etype == 'LINE':
        g['start'] = pt(10, 20)
        g['end'] = pt(11, 21)
    elif etype == 'CIRCLE':
        g['center'] = pt(10, 20)
        g['radius'] = as_float(last(40))
    elif etype == 'ARC':
        g['center'] = pt(10, 20)
        g['radius'] = as_float(last(40))
        g['start_angle'] = as_float(last(50))
        g['end_angle'] = as_float(last(51))
    elif etype == 'LWPOLYLINE':
        xs = allvals(10)
        ys = allvals(20)
        bulges = allvals(42)
        verts = []
        for i, x in enumerate(xs):
            y = ys[i] if i < len(ys) else None
            bulge = as_float(bulges[i]) if i < len(bulges) else None
            verts.append({'x': as_float(x), 'y': as_float(y), 'bulge': bulge})
        g['vertices'] = verts
        g['flags'] = as_int(last(70))
        g['closed'] = bool(g['flags'] & 1) if g['flags'] is not None else None
        g['const_width'] = as_float(last(43))
        g['elevation'] = as_float(last(38))
        g['thickness'] = as_float(last(39))
    elif etype == 'POLYLINE':
        g['location'] = pt(10, 20)
        g['flags'] = as_int(last(70))
    elif etype == 'TEXT':
        g['position'] = pt(10, 20)
        g['text'] = full_text()
        g['height'] = as_float(last(40))
        g['rotation'] = as_float(last(50))
    elif etype == 'MTEXT':
        g['position'] = pt(10, 20)
        g['text'] = full_text()
        g['height'] = as_float(last(40))
        g['style'] = decode_unicode(last(7))
        g['width'] = as_float(last(41))          # 字框宽度
        g['rect_height'] = as_float(last(46))
        g['attachment'] = as_int(last(71))
        g['flow_dir'] = as_int(last(72))
        g['linespace_factor'] = as_float(last(44))
    elif etype == 'INSERT':
        g['block'] = decode_unicode(last(2))
        g['insertion'] = pt(10, 20)
        g['scale_x'] = as_float(last(41))
        g['scale_y'] = as_float(last(42))
        g['rotation'] = as_float(last(50))
        g['columns'] = as_int(last(70))
        g['rows'] = as_int(last(71))
    elif etype == 'DIMENSION':
        g['block'] = decode_unicode(last(2))      # group 2 = anonymous block name
        g['dimstyle'] = decode_unicode(last(3))   # group 3 = dimension style name
        g['def_point'] = pt(10, 20)
        g['text_middle'] = pt(11, 21)
        g['xline1'] = pt(13, 23)
        g['xline2'] = pt(14, 24)
        g['dimtype'] = as_int(last(70))
        g['attach'] = as_int(last(71))
        g['measurement'] = as_float(last(42))
        g['bits_73_74_75'] = [as_int(last(73)), as_int(last(74)), as_int(last(75))]
        g['extrusion'] = pt(210, 220) if last(210) is not None else None
    elif etype == 'HATCH':
        g['pattern'] = decode_unicode(last(2))
        g['flags'] = as_int(last(70))
        g['associative'] = as_int(last(71))
        g['num_paths'] = as_int(last(91))
        # 92/93/72 每路径/每边重复出现，保序即为路径顺序
        g['path_flags'] = [as_int(v) for v in allvals(92)]
        g['path_counts'] = [as_int(v) for v in allvals(93)]
        g['edge_types'] = [as_int(v) for v in allvals(72)]
        g['style'] = as_int(last(75))
        g['pattern_type'] = as_int(last(76))
        g['num_def_lines'] = as_int(last(78))
        g['num_dashes'] = len(allvals(49))
        g['num_seeds'] = as_int(last(98))
    elif etype == 'VERTEX':
        g['position'] = pt(10, 20, 30)
        g['flags'] = as_int(last(70))
        g['start_width'] = as_float(last(40))
        g['end_width'] = as_float(last(41))
        g['bulge'] = as_float(last(42))
    elif etype == 'POINT':
        g['point'] = pt(10, 20)
    elif etype == 'ELLIPSE':
        g['center'] = pt(10, 20)
        g['major_axis'] = pt(11, 21)
        g['major_ratio'] = as_float(last(40))
    elif etype == 'SPLINE':
        g['flags'] = as_int(last(70))
        g['degree'] = as_int(last(71))
        g['knots'] = [as_float(v) for v in allvals(40)]
        g['num_fit'] = as_int(last(74))
        g['num_ctrl'] = as_int(last(73))
        xs = allvals(10)
        ys = allvals(20)
        zs = allvals(30)
        g['control'] = [
            [as_float(x), as_float(ys[i] if i < len(ys) else None),
             as_float(zs[i] if i < len(zs) else None)]
            for i, x in enumerate(xs)
        ]
        g['degree'] = as_int(last(71))
    elif etype in ('SOLID', 'TRACE', '3DFACE'):
        g['p1'] = pt(10, 20)
        g['p2'] = pt(11, 21)
        g['p3'] = pt(12, 22)
        g['p4'] = pt(13, 23)
    elif etype == 'ATTRIB' or etype == 'ATTDEF':
        g['position'] = pt(10, 20)
        g['text'] = full_text()
        g['height'] = as_float(last(40))
    return g


def collect_entity(etype, pairs):
    def last(code):
        vals = pairs.get(code)
        return vals[-1] if vals else None

    color = as_int(last(62))
    rec = {
        'handle': decode_unicode(last(5)),
        'type': etype,
        'layer': decode_unicode(last(8)),
        'color': color,
        'ltype': decode_unicode(last(6)),
        'space': None,          # model_space / block
        'block': None,          # 所属块名（块内部实体）
        'geometry': parse_geometry(etype, pairs),
    }
    return rec



def dict_objects(lines):
    """OBJECTS 段里的 DICTIONARY：数量 + 每个字典的键名（组码 3）。

    DXF 的字典键就是组码 3 的文本；这里是真值侧唯一的字典来源，
    解析器侧的 DICTIONARY（DWG 类型 0x2A）与之对数量与键名。
    """
    inside = False
    recs = []
    cur = None
    pending_type = None
    i = 0
    n = len(lines)
    while i + 1 < n:
        c = lines[i].strip()
        v = lines[i + 1].rstrip('\r')
        i += 2
        if not c:
            continue
        try:
            code = int(c)
        except ValueError:
            continue
        if code == 0 and v == 'SECTION':
            pending_type = 'SECTION_HEAD'
            continue
        if pending_type == 'SECTION_HEAD' and code == 2:
            inside = (v == 'OBJECTS')
            pending_type = None
            continue
        if code == 0 and v == 'ENDSEC':
            inside = False
            continue
        if not inside:
            continue
        if code == 0:
            if cur is not None:
                recs.append(cur)
            cur = {'type': v, 'keys': [], 'handle': None}
            continue
        if cur is None:
            continue
        if code == 3:
            cur['keys'].append(v)
        elif code == 5:
            cur['handle'] = v
    if cur is not None:
        recs.append(cur)
    dicts = [r for r in recs if r['type'] == 'DICTIONARY']
    return {
        'count': len(dicts),
        'key_total': sum(len(d['keys']) for d in dicts),
        'keys': sorted(k for d in dicts for k in d['keys']),
    }


def class_objects(lines):
    """CLASSES 段：类表（类号 / DXF 名 / C++ 名 / 应用名 / item 类号）。

    DXF writer 把 DWG 的 AcDb:Classes 原样导成 CLASSES 段，因此这是类表
    解码唯一的外部真值：组码 1/2/3 是 DXF 名/C++ 名/应用名，281 指出该类
    是实体(1) 还是对象(0)。DXF 不导类号（那是 DWG 内部的 500+ 索引）。
    """
    inside = False
    out = []
    cur = None
    i = 0
    n = len(lines)
    while i + 1 < n:
        c = lines[i].strip()
        v = lines[i + 1].rstrip('\r')
        i += 2
        if not c:
            continue
        try:
            code = int(c)
        except ValueError:
            continue
        if code == 0 and v == 'SECTION':
            cur = None
            continue
        if code == 2 and cur is None and v == 'CLASSES':
            inside = True
            continue
        if code == 0 and v == 'ENDSEC':
            if cur is not None:
                out.append(cur)
                cur = None
            inside = False
            continue
        if not inside:
            continue
        if code == 0 and v == 'CLASS':
            if cur is not None:
                out.append(cur)
            cur = {'dxf': '', 'cpp': '', 'app': '',
                   'proxy_flags': None, 'is_entity': None}
            continue
        if cur is None:
            continue
        if code == 1:
            cur['dxf'] = v
        elif code == 2:
            cur['cpp'] = v
        elif code == 3:
            cur['app'] = v
        elif code == 90:
            cur['proxy_flags'] = as_int(v)
        elif code == 281:
            cur['is_entity'] = as_int(v)
    return {
        'count': len(out),
        'dxf_names': sorted(x['dxf'] for x in out if x['dxf']),
        'rows': out,
    }


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    dxf_path, json_path = argv[1], argv[2]

    # 先探测编码：DXF 可能为 UTF-8 或本地代码页；按字节读，优先 UTF-8 严格解码。
    with open(dxf_path, 'rb') as f:
        raw = f.read()
    if raw[:3] == b'\xef\xbb\xbf':
        raw = raw[3:]
    try:
        text = raw.decode('utf-8')
        encoding_used = 'utf-8'
    except UnicodeDecodeError:
        text = raw.decode('gbk', errors='replace')
        encoding_used = 'gbk-replace'
    lines = text.splitlines()

    header = {}
    header_values = {}
    header_var = None
    layers = []
    blocks = []
    styles = []
    dimstyles = []
    linetypes = []
    pending = None
    appids = []
    entities = []

    section = None
    in_table = None
    cur_block_name = None
    block_meta = {}
    hdr = None
    codepage = None
    acadver = None

    pairs = None
    pending_etype = None

    def flush_entity():
        nonlocal pending_etype, pairs
        if pending_etype is not None and pairs is not None:
            rec = collect_entity(pending_etype, pairs)
            rec['space'] = 'model_space' if section == 'ENTITIES' else 'block'
            if section == 'BLOCKS':
                rec['block'] = cur_block_name
            entities.append(rec)
        pending_etype = None
        pairs = None

    i = 0
    n = len(lines)
    while i + 1 < n:
        code_raw = lines[i].strip()
        if code_raw == '':
            i += 1
            continue
        try:
            code = int(code_raw)
        except ValueError:
            i += 1
            continue
        value = lines[i + 1].rstrip('\r')
        i += 2

        if code == 0 and value == 'SECTION':
            flush_entity()
            section = None
            continue
        if code == 2 and section is None:
            section = value
            continue

        if section == 'HEADER':
            # 组码 9 引导变量名（$EXTMIN 等）；其后各组的第一个值即为该变量值。
            if code == 9:
                header_var = value.strip()
                header_values.setdefault(header_var, [])
            elif header_var:
                header_values[header_var].append((code, value))
            if code == 1 and value.startswith('AC10'):
                acadver = value
            continue

        if section == 'TABLES':
            if code == 0 and value == 'TABLE':
                in_table = None
                continue
            if code == 2 and in_table is None:
                in_table = value
                continue
            if code == 0 and value == 'ENDTAB':
                in_table = None
                continue

            if in_table == 'LAYER' and code == 0:
                # 新图层记录开始；记录由后续 2/70/62/6 填充
                layers.append({'name': None, 'color': None, 'ltype': None,
                               'flags': None, 'frozen': False, 'locked': False, 'off': False})
                continue
            if in_table == 'LAYER' and layers:
                cur = layers[-1]
                if code == 2:
                    cur['name'] = decode_unicode(value)
                elif code == 70:
                    cur['flags'] = as_int(value)
                    f = cur['flags'] or 0
                    cur['frozen'] = bool(f & 1)
                    cur['locked'] = bool(f & 4)
                elif code == 62:
                    ci = as_int(value)
                    cur['color'] = ci
                    cur['off'] = ci is not None and ci < 0
                elif code == 6:
                    cur['ltype'] = decode_unicode(value)
                continue

            if in_table == 'LTYPE':
                # 组码 49/74/75/46/50/44/45 属于同一个图案段，按段累积。
                if code == 0:
                    linetypes.append({'name': None, 'description': None,
                                      'pattern_len': None, 'alignment': None,
                                      'dashes': []})
                    dash = {'length': None, 'flags': None, 'shapecode': None,
                            'scale': None, 'rotation': None, 'x_offset': None,
                            'y_offset': None}
                    pending = dash
                    continue
                if linetypes:
                    cur = linetypes[-1]
                    if code == 2:
                        cur['name'] = decode_unicode(value)
                    elif code == 3:
                        cur['description'] = decode_unicode(value)
                    elif code == 40:
                        cur['pattern_len'] = as_float(value)
                    elif code == 72:
                        cur['alignment'] = as_int(value)
                    elif code == 49:
                        dash = {'length': as_float(value), 'flags': None,
                                'shapecode': None, 'scale': None,
                                'rotation': None, 'x_offset': None,
                                'y_offset': None}
                        cur['dashes'].append(dash)
                        pending = dash
                    elif pending is not None and code == 74:
                        pending['flags'] = as_int(value)
                    elif pending is not None and code == 75:
                        pending['shapecode'] = as_int(value)
                    elif pending is not None and code == 46:
                        pending['scale'] = as_float(value)
                    elif pending is not None and code == 50:
                        pending['rotation'] = as_float(value)
                    elif pending is not None and code == 44:
                        pending['x_offset'] = as_float(value)
                    elif pending is not None and code == 45:
                        pending['y_offset'] = as_float(value)
                continue

            if in_table in ('STYLE', 'DIMSTYLE', 'APPID'):
                target = {'STYLE': styles, 'DIMSTYLE': dimstyles, 'APPID': appids}[in_table]
                if code == 0:
                    rec = {'name': None, 'flags': None}
                    if in_table == 'STYLE':
                        rec.update({'font': None, 'bigfont': None, 'height': None,
                                    'width': None, 'oblique': None})
                    target.append(rec)
                    continue
                # 首个 0/2 之前的记录由上面 code==0 建好
                if target:
                    cur = target[-1]
                    if code == 2:
                        cur['name'] = decode_unicode(value)
                    elif code == 70:
                        cur['flags'] = as_int(value)
                    elif in_table == 'STYLE':
                        if code == 40:
                            cur['height'] = as_float(value)
                        elif code == 41:
                            cur['width'] = as_float(value)
                        elif code == 50:
                            cur['oblique'] = as_float(value)
                        elif code == 3:
                            cur['font'] = decode_unicode(value)
                        elif code == 4:
                            cur['bigfont'] = decode_unicode(value)
                continue

            if in_table in ('BLOCK', 'BLOCK_RECORD') and code == 0:
                blocks.append({'name': None, 'base_point': [None, None, None],
                               'anonymous': False, 'flags': None})
                continue
            if in_table in ('BLOCK', 'BLOCK_RECORD') and blocks:
                cur = blocks[-1]
                if code == 2:
                    cur['name'] = decode_unicode(value)
                elif code == 70:
                    cur['flags'] = as_int(value)
                    cur['anonymous'] = bool((cur['flags'] or 0) & 1)
                elif code == 10:
                    cur['base_point'][0] = as_float(value)
                elif code == 20:
                    cur['base_point'][1] = as_float(value)
                elif code == 30:
                    cur['base_point'][2] = as_float(value)
                continue
            continue

        if section == 'BLOCKS':
            if code == 0 and value == 'BLOCK':
                flush_entity()
                cur_block_name = None
                hdr = {'name': None, 'pt': [None, None, None], 'flags': None}
                continue
            if code == 0 and value == 'ENDBLK':
                flush_entity()
                if hdr is not None and hdr['name'] is not None:
                    block_meta[hdr['name']] = hdr
                hdr = None
                continue
            if code == 0:
                flush_entity()
                if value in ('SEQEND', 'ENDSEC', 'EOF', 'SECTION'):
                    continue
                pending_etype = value
                pairs = {}
                continue
            if hdr is not None and pending_etype is None:
                if code == 2:
                    hdr['name'] = decode_unicode(value)
                    cur_block_name = hdr['name']
                elif code == 70:
                    hdr['flags'] = as_int(value)
                elif code == 10:
                    hdr['pt'][0] = as_float(value)
                elif code == 20:
                    hdr['pt'][1] = as_float(value)
                elif code == 30:
                    hdr['pt'][2] = as_float(value)
                continue
            if pairs is not None:
                pairs.setdefault(code, []).append(value)
            continue

        if section == 'ENTITIES':
            if code == 0:
                flush_entity()
                if value in ('ENDSEC', 'EOF', 'SECTION'):
                    continue
                pending_etype = value
                pairs = {}
                continue
            if pairs is not None:
                pairs.setdefault(code, []).append(value)
            continue

    flush_entity()

    # 用 BLOCKS 段头中的基点补齐块表记录
    known = {b['name'] for b in blocks}
    for name, meta in block_meta.items():
        if name in known:
            for b in blocks:
                if b['name'] == name:
                    b['base_point'] = meta['pt']
                    if b['flags'] is None:
                        b['flags'] = meta['flags']
                        b['anonymous'] = bool((meta['flags'] or 0) & 1)
        else:
            blocks.append({'name': name, 'base_point': meta['pt'],
                           'anonymous': bool((meta['flags'] or 0) & 1),
                           'flags': meta['flags']})

    model = [e for e in entities if e['space'] == 'model_space']
    inblk = [e for e in entities if e['space'] == 'block']

    def counts(seq):
        c = {}
        for e in seq:
            c[e['type']] = c.get(e['type'], 0) + 1
        return dict(sorted(c.items()))

    def header_simple(entries):
        """把 $(变量) 的组值整理成可比的标量/向量/句柄字符串。"""
        out = {}
        for name, items in entries.items():
            codes = [c for c, _ in items]
            if 10 in codes and 20 in codes:
                x = y = z = None
                for c, v in items:
                    if c == 10:
                        x = as_float(v)
                    elif c == 20:
                        y = as_float(v)
                    elif c == 30:
                        z = as_float(v)
                out[name] = [x, y, z]
            elif codes and codes[0] in (5, 330, 340, 350, 360):
                out[name] = items[0][1]
            elif codes and codes[0] == 1:
                out[name] = decode_unicode(items[0][1])
            elif codes and codes[0] == 3:
                out[name] = decode_unicode(items[0][1])
            elif codes:
                out[name] = as_float(items[0][1])
        return out

    truth = {
        'source_dxf': dxf_path,
        'encoding_used': encoding_used,
        'dxf_version': acadver,
        'dwg_codepage': codepage,
        'summary': {
            'model_space_entities': len(model),
            'block_internal_entities': len(inblk),
            'type_counts': counts(entities),
            'type_counts_model_space': counts(model),
            'type_counts_in_blocks': counts(inblk),
            'handle_count_unique': len({e['handle'] for e in entities if e['handle']}),
        },
        'dictionaries': dict_objects(lines),
        'classes': class_objects(lines),
        'header_vars': header_simple(header_values),
        'layers': layers,
        'linetypes': linetypes,
        'blocks': blocks,
        'styles': styles,
        'dimstyles': dimstyles,
        'appids': appids,
        'entities': entities,
    }

    with open(json_path, 'w', encoding='utf-8', newline='\n') as f:
        json.dump(truth, f, ensure_ascii=False, indent=1)
    print('entities=%d (model=%d block=%d) layers=%d blocks=%d styles=%d dimstyles=%d appids=%d -> %s'
          % (len(entities), len(model), len(inblk), len(layers), len(blocks),
             len(styles), len(dimstyles), len(appids), json_path))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
