# -*- coding: utf-8 -*-
"""配电干线图的规则引擎（数据/口径与界面分离）。

设计：powerplan.rules.json 存全部可调参数（几何、电压分档、状态栏文案、表头列宽、
小地图形制、字号），本模块只做"读规则 + 纯函数计算"。界面（XML 或现 ppApp）
只负责展示与把事件转成命令，不再散落常量。

这样做的直接好处：
  * 改电压口径/加档位，只改 json；
  * 复核时可以把 rules 换一份（不同项目一套参数），界面不用动；
  * 计算是纯函数，能在测试里直接断言，不必开窗口。
"""
import json
import os
from functools import lru_cache

DEFAULT_RULES = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'powerplan.rules.json')

# 双电源：起点列可写多个编号，用这些字符分隔；第一个是主电源（入树），其余只补进线
UPSTREAM_SEPS = '|,，、;；/+'


@lru_cache(maxsize=8192)
def split_upstream(raw):
    """把'TR1|TR2'这类多值上游拆成编号列表（已去空白）。"""
    s = (raw or '').strip()
    if not s:
        return []
    for ch in UPSTREAM_SEPS:
        s = s.replace(ch, '\x01')
    return [t.strip() for t in s.split('\x01') if t.strip()]


@lru_cache(maxsize=8192)
def primary_upstream(raw):
    ups = split_upstream(raw)
    return ups[0] if ups else ''


import math
import re


def calc_length(h, v, margin):
    """计算书口径：合计 = 水平 + 垂直 + 余量，向上取整到整米。"""
    return float(math.ceil(float(h or 0) + float(v or 0) + float(margin or 0)))


def _norm_len(f):
    """长度口径（用户计算书）：合计 = ceil(水平 + 垂直 + 余量)。
    老数据只有合计时，记作水平长度、垂直 0、余量 0。"""
    h = f.get('hLen')
    v = f.get('vLen')
    m = f.get('margin')
    L = f.get('length')
    if h is None and v is None:
        return float(L or 0), 0.0, 0.0, float(L or 0)
    h = float(h or 0)
    v = float(v or 0)
    m = float(m or 0)
    return h, v, m, calc_length(h, v, m)


def _norm_feed(f):
    h, v, m, L = _norm_len(f)
    return {'upstream': (f.get('upstream') or '').strip(),
            'circuit': (f.get('circuit') or '').strip(),
            'cable': (f.get('cable') or '').strip(),
            'strands': int(float(f.get('strands') or 1)),
            'hLen': h, 'vLen': v, 'margin': m, 'length': L}


# 一次布局/一次场景里，同一节点的进线会被问 6 次（卡片尺寸、连线、双电源 extras…）。
# 缓存只在单次 pass 内有效，pass 开始清空——不跨 rebuild，避免表格改值后读到旧数据。
_FEEDS_CACHE = {}


def feeds_begin():
    _FEEDS_CACHE.clear()


def feeds_of(node):
    """一个卡片（节点）的进线列表，每条 = {upstream, circuit, cable, strands, hLen, vLen, length}。

    双电源柜在画布上仍是一张卡片，但表格/CSV 一条进线一行：
    两条进线各有自己的回路编号、电缆型号、拼数与水平/垂直/合计长度。
    有 node['feeds'] 就用它（表格编辑/导入后的规范形态）；
    没有就按节点字段合成——upstream 写多值时，多余进线沿用同一套电缆参数。
    """
    hit = _FEEDS_CACHE.get(id(node))
    if hit is not None and hit[0] is node:
        return hit[1]
    res = _feeds_compute(node)
    _FEEDS_CACHE[id(node)] = (node, res)
    return res


def _feeds_compute(node):
    fs = node.get('feeds') or []
    if fs:
        return [_norm_feed(f) for f in fs]
    base = {'circuit': node.get('circuit') or '',
            'cable': node.get('cable') or '',
            'strands': node.get('strands') or 1,
            'hLen': node.get('hLen'), 'vLen': node.get('vLen'),
            'margin': node.get('margin'), 'length': node.get('length')}
    ups = split_upstream(node.get('upstream'))
    out = []
    for u in (ups or ['']):
        f = dict(base)
        f['upstream'] = u
        out.append(_norm_feed(f))
    return out

class Rules:
    def __init__(self, path=None):
        self.path = path or DEFAULT_RULES
        with open(self.path, 'r', encoding='utf-8') as f:
            self.data = json.load(f)

    def get(self, *keys, default=None):
        cur = self.data
        for k in keys:
            if not isinstance(cur, dict) or k not in cur:
                return default
            cur = cur[k]
        return cur

    # --- 便捷访问 ---
    @property
    def cfg(self):
        return self.data.get('cfg', {})

    @property
    def zoom(self):
        return self.data.get('zoom', {})

    @property
    def voltage(self):
        return self.data.get('voltage', {})

    @property
    def status(self):
        return self.data.get('status', {})

    @property
    def table(self):
        return self.data.get('table', {})

    @property
    def cable(self):
        return self.data.get('cable', {})

    @property
    def mini(self):
        return self.data.get('mini', {})


class PowerPlanEngine:
    """所有"自动计算"都走这里：电压归一/分档/文案、卡片尺寸、电缆标注、
    层级与下游、编号建议、状态栏文案、小地图几何。 """

    def __init__(self, rules=None):
        self.rules = rules if isinstance(rules, Rules) else Rules(rules)

    # ---------------- 电压 ----------------
    @lru_cache(maxsize=4096)
    def normVoltage(self, v):
        vt = self.rules.voltage
        v = (v or '').strip()
        if not v:
            return vt.get('default', '380')
        low = v.lower().replace(' ', '')
        for unit in vt.get('kvUnits', ['kv', '千伏']):
            if unit in low:
                # 取单位前面的数字，"10kV 中压" → 10kV
                num = low.split(unit)[0]
                m = re.search(r'(\d+(?:\.\d+)?)', num)
                if m:
                    return ('%g' % float(m.group(1))) + 'kV'
                return vt.get('default', '380')
        for token in vt.get('threePhase', ['380', '三相', '3ph']):
            if token in low:
                return '380'
        for token in vt.get('singlePhase', ['220', '单相', '1ph']):
            if token in low:
                return '220'
        return vt.get('default', '380')

    def volt_class(self, nv):
        """返回 lv380 / lv220 / mv / hv 四档之一。"""
        vt = self.rules.voltage
        base = vt.get('lvBase', ['380', '220'])
        if nv == '380':
            return 'lv380'
        if nv == '220':
            return 'lv220'
        try:
            num = float(str(nv).replace('kV', ''))
        except ValueError:
            return 'lv380'
        return 'hv' if num >= vt.get('hvMin', 66) else 'mv'

    def voltBucket(self, v):
        """兼容旧接口的 0..3：380 / 220 / mv / hv。"""
        return {'lv380': 0, 'lv220': 1, 'mv': 2, 'hv': 3}[self.volt_class(self.normVoltage(v))]

    def voltText(self, v):
        nv = self.normVoltage(v)
        labels = self.rules.voltage.get('labels', {})
        if nv in labels:
            return labels[nv]
        return nv + (' 高压' if self.volt_class(nv) == 'hv' else ' 中压')

    @lru_cache(maxsize=4096)
    def volt_style(self, v):
        cls = self.volt_class(self.normVoltage(v))
        st = self.rules.voltage.get('styles', {}).get(cls, {})
        text = st.get('text') or self.normVoltage(v)
        return {'text': text, 'fg': st.get('fg', '#1F64B0'), 'bg': st.get('bg', '#E8F1FB')}

    def voltage_options(self, values=()):
        """基档 + 数据里出现的非标准值（按 HTML voltageOptions 的顺序）。"""
        opts = list(self.rules.voltage.get('lvBase', ['380', '220']))
        opts += list(self.rules.voltage.get('extraOptions', ['10kV', '35kV', '110kV']))
        for v in values:
            nv = self.normVoltage(v)
            if nv not in opts:
                opts.append(nv)
        return opts

    # ---------------- 卡片与电缆 ----------------
    @lru_cache(maxsize=4096)
    def text_units(self, s):
        # 10221 张卡片每张量 3 行文字（5 万次 genexpr + 44 万次 ord）；
        # 卡片文字高度重复（型号/长度/电压就那么几种），直接记忆化
        return sum(2 if ord(ch) >= 0x2E80 else 1 for ch in (s or ''))

    def card_size(self, node):
        """按 HTML cardSize(role, hasName, lines) 的口径算卡片尺寸。"""
        return self._card_size_by_role(self.role_of(node), bool((node.get('name') or '').strip()),
                                       [node.get('name') or node.get('code') or '',
                                        self.power_volt_line(node), self.cable_len_line(node)])

    def role_of(self, node):
        up = (node.get('upstream') or '').strip()
        if not up:
            return 'root'
        if node.get('_children_count') or float(node.get('collapsedCount') or 0):
            return 'branch'
        return 'terminal'

    def _card_size_by_role(self, role, has_name, lines):
        return self._card_size_cached(role, has_name, tuple(lines))

    @lru_cache(maxsize=8192)
    def _card_size_cached(self, role, has_name, lines):
        cfg = self.rules.cfg
        base_w = cfg.get('termW', 120) if role == 'terminal' else cfg.get('boxW', 132)
        base_h = cfg.get('termH', 44) if role == 'terminal' else cfg.get('boxH', 48)
        max_u = max([self.text_units(t) for t in lines if t] or [0])
        # 正文默认两行高；第三行（电缆型号 + 合计长度）再往下加 13px
        extra = 13 * max(0, len(lines) - 2)
        return (max(base_w, 20 + max_u * 8),
                base_h + (cfg.get('nameH', 8) if has_name else 0) + extra)

    def cable_len_line(self, node):
        """卡片第三行：电缆型号 · 合计长度（如 "YJV22-4×35 · 11 m"）。"""
        f = (feeds_of(node) or [{}])[0]
        return self._cll_cached((f.get('cable') or '').strip(), float(f.get('length') or 0))

    @lru_cache(maxsize=8192)
    def _cll_cached(self, cable, L):
        s = cable
        if L:
            s = (s + ' ' if s else '') + '%g m' % L
        return s

    def power_volt_line(self, node):
        """卡片第二行：功率 · 电压等级（如 "11 kW · 380V"）。"""
        return self._pvl_cached(float(node.get('loadKw') or 0), node.get('voltage'), self.volt_style(node.get('voltage'))['text'])

    @lru_cache(maxsize=8192)
    def _pvl_cached(self, load, volt_raw, volt_text):
        s = ''
        if load:
            s = '%g kW' % load
        v = volt_text
        if v:
            s += (' · ' if s else '') + v
        return s

    def cable_line(self, node):
        cb = self.rules.cable
        parts = []
        if (node.get('cable') or '').strip():
            parts.append(node['cable'].strip())
        load = float(node.get('loadKw') or 0)
        if load:
            parts.append('%g%s' % (load, cb.get('loadSuffix', ' kW')))
        length = float(node.get('length') or 0)
        if length:
            parts.append('%g%s' % (length, cb.get('lengthSuffix', ' m')))
        return cb.get('sep', ' · ').join(parts)

    def cable_label_w(self, cable):
        """电缆标注在画布上占的宽度（10px 字号：半角≈5.6，CJK 记 2 单位 + 留白）。"""
        s = (cable or '').strip()
        return 0 if not s else self.text_units(s) * 6 + 10

    # ---------------- 图结构 ----------------
    @staticmethod
    def _children_map(nodes):
        m = {}
        codes = set(n.get('code') for n in nodes)
        for n in nodes:
            # 双电源只按主电源挂树；备用上游不参与树结构
            for up in split_upstream(n.get('upstream')):
                if up in codes and up != n.get('code'):
                    m.setdefault(up, []).append(n.get('code'))
                    break
        return m

    def childrenMap(self, nodes):
        return self._children_map(nodes)

    def descendants(self, nodes, code):
        m = self._children_map(nodes)
        out, stack = [], [code]
        while stack:
            cur = stack.pop()
            for c in m.get(cur, []):
                out.append(c)
                stack.append(c)
        return out

    def depthOf(self, nodes, code):
        by = {n.get('code'): n for n in nodes}
        d, cur, guard = 0, code, 0
        while cur in by and guard < 1000:
            up = primary_upstream(by[cur].get('upstream'))
            if not up or up not in by:
                break
            d += 1
            cur = up
            guard += 1
        return d

    def suggest_code(self, nodes, parent=None):
        codes = set(n.get('code') for n in nodes)
        if parent:
            i = 1
            while ('%s-%d' % (parent, i)) in codes:
                i += 1
            return '%s-%d' % (parent, i)
        i = 1
        while ('N%d' % i) in codes:
            i += 1
        return 'N%d' % i

    def nodeOf(self, nodes, code):
        for n in nodes:
            if n.get('code') == code:
                return n
        return None

    # ---------------- 统计与文案 ----------------
    def countOverlaps(self, boxes):
        n = 0
        for i in range(len(boxes or [])):
            a = boxes[i]
            for j in range(i + 1, len(boxes or [])):
                b = boxes[j]
                if (a['x'] < b['x'] + b['w'] and b['x'] < a['x'] + a['w'] and
                        a['y'] < b['y'] + b['h'] and b['y'] < a['y'] + a['h']):
                    n += 1
        return n

    def statusText(self, view, nodes, zoom, overlaps=0):
        st = self.rules.status
        cnt = {'lv380': 0, 'lv220': 0, 'mv': 0, 'hv': 0}
        for n in nodes:
            cnt[self.volt_class(self.normVoltage(n.get('voltage')))] += 1
        zoom_txt = '%d%%' % round(zoom * 100)
        hint = st.get('wideHint', '') if zoom < self.rules.zoom.get('hintBelow', 0.55) else ''
        seg = st.get('sep', ' / ')
        volt_parts = ['380V %d' % cnt['lv380'], '220V %d' % cnt['lv220']]
        if cnt['mv']:
            volt_parts.append('中压 %d' % cnt['mv'])
        if cnt['hv']:
            volt_parts.append('高压 %d' % cnt['hv'])
        item = st.get('item', ' · ')
        parts = [
            '%s %d' % (st.get('icon', '节点'), len(view.get('nodes') or nodes)),
            '%s %d' % (st.get('link', '连线'), len(view.get('links') or [])),
            '%s %d' % (st.get('cross', '交叉'), view.get('crossings', 0)),
            '%s %d' % (st.get('overlap', '重叠'), overlaps),
            seg.join(volt_parts),
            zoom_txt + hint,
        ]
        return item.join(parts) + st.get('tail', '')

    # ---------------- 排版（HTML layoutPlan，纯 Python；C++ 只按坐标画） ----------------
    def layout(self, nodes, horizontal=False):
        feeds_begin()
        cfg = self.rules.cfg
        gap_x, gap_y = cfg.get('gapX', 30), cfg.get('gapY', 56)
        margin, root_gap = cfg.get('margin', 24), cfg.get('rootGap', 64)
        cm = self._children_map(nodes)
        codes = set(n.get('code') for n in nodes)
        items, roots = {}, []
        for n in nodes:
            code = n.get('code')
            if code in items:
                continue
            up = (n.get('upstream') or '').strip()
            kids = cm.get(code, [])
            collapsed = float(n.get('collapsedCount') or 0)
            is_root = (not up) or up not in codes or up == code
            role = 'root' if is_root else ('branch' if (kids or collapsed) else 'terminal')
            # 卡片正文口径与画布一致：设备名称 / 功率·电压等级
            body = [((n.get('name') or '').strip() or (code or '')),
                    self.power_volt_line(n), self.cable_len_line(n)]
            w, h = self._card_size_by_role(role, bool((n.get('name') or '').strip()), body)
            items[code] = {'code': code, 'name': n.get('name') or '', 'cable': n.get('cable') or '',
                           'voltage': n.get('voltage') or '', 'role': role, 'w': w, 'h': h,
                           'depth': 0, 'children': kids, 'x': 0.0, 'y': 0.0, 'subW': 0,
                           'extras': [dict(f) for f in feeds_of(n)[1:]
                                      if f['upstream'] in codes and f['upstream'] != code]}
            # 双电源柜：第二个及以上上游 → 卡片红、它的回路画洋红
            items[code]['dual'] = 1 if items[code]['extras'] else 0
            if role == 'root':
                roots.append(code)
        # 深度语义与 depthOf 完全一致（沿 primary upstream 逐级往上数），
        # 但 {code:node} 只建一次 + 深度记忆化：大图不再 O(n²)。
        byCode = {n.get('code'): n for n in nodes}
        depthMemo = {}
        for code in items:
            chain, cur, guard = [], code, 0
            while cur not in depthMemo and guard < 1000:
                n = byCode.get(cur)
                if n is None:
                    break
                up = primary_upstream(n.get('upstream'))
                if not up or up not in byCode:
                    break
                chain.append(cur)
                cur = up
                guard += 1
            base = depthMemo.get(cur, 0)
            for c in reversed(chain):
                base += 1
                depthMemo[c] = base
            items[code]['depth'] = depthMemo.get(code, 0)

        # 子树主轴尺寸：纵向按宽累加，横向按高累加
        def calc_sub(code, guard=0):
            it = items[code]
            own = it['h'] if horizontal else it['w']
            if not it['children'] or guard > 200:
                it['subW'] = own
                return own
            total = 0
            for i, c in enumerate(it['children']):
                if c not in items:
                    continue
                total += calc_sub(c, guard + 1) + (gap_x if i else 0)
            it['subW'] = max(own, total)
            return it['subW']

        for r in roots:
            calc_sub(r)

        def place(code, start, guard=0):
            it = items[code]
            it['treeLeft'] = start
            kids = [c for c in it['children'] if c in items]
            if not kids or guard > 200:
                if horizontal:
                    it['y'] = start + (it['subW'] - it['h']) / 2.0
                else:
                    it['x'] = start + (it['subW'] - it['w']) / 2.0
                return
            ext = sum(items[c]['subW'] + (gap_x if i else 0) for i, c in enumerate(kids))
            cur = start + (it['subW'] - ext) / 2.0
            first = last = None
            for c in kids:
                place(c, cur, guard + 1)
                if first is None:
                    first = c
                last = c
                cur += items[c]['subW'] + gap_x
            if horizontal:
                mid = (items[first]['y'] + items[first]['h'] / 2.0 +
                       items[last]['y'] + items[last]['h'] / 2.0) / 2.0
                it['y'] = mid - it['h'] / 2.0
            else:
                mid = (items[first]['x'] + items[first]['w'] / 2.0 +
                       items[last]['x'] + items[last]['w'] / 2.0) / 2.0
                it['x'] = mid - it['w'] / 2.0

        cursor_root = margin
        for r in roots:
            place(r, cursor_root)
            cursor_root += items[r]['subW'] + root_gap

        # 层级坐标（纵向=行 y，横向=列 x），同列/同层对齐
        by_depth = {}
        max_d = 0
        for it in items.values():
            by_depth.setdefault(it['depth'], []).append(it)
            max_d = max(max_d, it['depth'])
        cursor = margin
        for d in range(0, max_d + 1):
            bucket = by_depth.get(d, [])
            if horizontal:
                col_w = max([x['w'] for x in bucket] or [cfg.get('boxW', 132)])
                # 列间距要留得下"进入卡片的那段横线 + 电缆标注"，否则标注会压卡片
                lab = max([self.cable_label_w(x['cable']) for x in bucket] or [0])
                for it in bucket:
                    it['x'] = cursor
                cursor += col_w + max(100, lab + 16)
            else:
                rowH = max([x['h'] for x in bucket] or [cfg.get('boxH', 48)])
                lab = max([self.cable_label_w(x['cable']) for x in bucket] or [0])
                for it in bucket:
                    it['y'] = cursor + (rowH - it['h']) / 2.0
                cursor += rowH + max(gap_y, lab + 16)

        out = sorted(items.values(), key=lambda x: x['code'])
        out_nodes = [{'code': x['code'], 'x': x['x'], 'y': x['y'], 'w': x['w'], 'h': x['h'],
                      'depth': x['depth'], 'dual': int(x.get('dual') or 0)} for x in out]
        links = []
        for it in out:
            for c in it['children']:
                if c not in items:
                    continue
                ch = items[c]
                if horizontal:
                    x1, y1 = it['x'] + it['w'], it['y'] + it['h'] / 2.0
                    x2, y2 = ch['x'], ch['y'] + ch['h'] / 2.0
                    mx = x2 - max((x2 - x1) / 2.0, self.cable_label_w(ch['cable']) + 10)
                    mx = max(mx, x1 + 6)
                    bez = [x1, y1, mx, y1, mx, y2, x2, y2]
                else:
                    x1, y1 = it['x'] + it['w'] / 2.0, it['y'] + it['h']
                    x2, y2 = ch['x'] + ch['w'] / 2.0, ch['y']
                    my = y2 - max((y2 - y1) / 2.0, self.cable_label_w(ch['cable']) + 10)
                    my = max(my, y1 + 6)
                    bez = [x1, y1, x1, my, x2, my, x2, y2]
                links.append({'from': it['code'], 'to': c, 'cable': ch['cable'],
                              'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'bezier': bez,
                              'vertical': 0 if horizontal else 1, 'dashed': 0,
                              'magenta': 1 if it.get('dual') else 0})
        # 双电源：备用电源各补一条虚线进线（不参与布局，也不标电缆）
        for it in out:
            for pin in it.get('extras', []):
                pi = pin.get('upstream') or ''
                pb = items.get(pi)
                if pb is None:
                    continue
                if horizontal:
                    x1, y1 = pb['x'] + pb['w'], pb['y'] + pb['h'] / 2.0
                    x2, y2 = it['x'], it['y'] + min(10, it['h'] / 3.0)
                    mx = max(x2 - max((x2 - x1) / 2.0, 24.0), x1 + 6)
                    bez = [x1, y1, mx, y1, mx, y2, x2, y2]
                else:
                    x1, y1 = pb['x'] + pb['w'] / 2.0, pb['y'] + pb['h']
                    x2, y2 = it['x'] + min(10, it['w'] / 3.0), it['y']
                    my = max(y2 - max((y2 - y1) / 2.0, 24.0), y1 + 6)
                    bez = [x1, y1, x1, my, x2, my, x2, y2]
                links.append({'from': pi, 'to': it['code'],
                              'cable': (pin.get('cable') or '').strip(),
                              'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'bezier': bez,
                              'vertical': 0 if horizontal else 1, 'dashed': 1, 'magenta': 1})
        links.sort(key=lambda l: (l['from'], l['to']))
        max_x = max([x['x'] + x['w'] for x in out] or [0])
        max_y = max([x['y'] + x['h'] for x in out] or [0])
        width = (max_x if out else 0) + margin
        height = (max_y if out else 0) + margin
        steps = cfg.get('curveSteps', 16)
        return {'width': width, 'height': height, 'dir': 'h' if horizontal else 'v',
                'nodes': out_nodes, 'boxes': out_nodes, 'links': links,
                'crossings': self.countCrossings(links, steps),
                'byDepth': by_depth}

    # ---------------- 场景 JSON（给 C++ 画布：只有坐标，没有布局算法） ----------------
    def scene(self, nodes, horizontal=False, title=''):
        """产出与 powerplan.toJson 同构的场景：nodes（业务字段）+ layout（坐标）。
        C++ 侧 parseScene 直接按坐标画，不再自己排版。"""
        feeds_begin()
        lay = self.layout(nodes, horizontal)
        cm = self._children_map(nodes)
        codes = set(n.get('code') for n in nodes)
        lay_nodes = {x['code']: x for x in lay['nodes']}
        out_nodes = []
        for n in nodes:
            code = n.get('code')
            if code not in lay_nodes:
                continue
            up = (n.get('upstream') or '').strip()
            kids = cm.get(code, [])
            prim = primary_upstream(up)
            is_root = (not prim) or prim not in codes or prim == code
            role = 'root' if is_root else ('branch' if (kids or float(n.get('collapsedCount') or 0)) else 'terminal')
            out_nodes.append({
                'code': code, 'name': n.get('name') or '', 'upstream': up,
                'cable': n.get('cable') or '', 'loadKw': float(n.get('loadKw') or 0),
                'voltage': n.get('voltage') or '380', 'length': float(n.get('length') or 0),
                'role': role, 'collapsedCount': int(n.get('collapsedCount') or 0),
                'dual': 1 if lay_nodes[code].get('dual') else 0})
        return {
            'title': title,
            'dir': 'h' if horizontal else 'v',
            'nodes': out_nodes,
            'layout': {
                'width': int(round(lay['width'])),
                'height': int(round(lay['height'])),
                'nodes': [{'code': b['code'], 'x': int(round(b['x'])), 'y': int(round(b['y'])),
                           'w': int(round(b['w'])), 'h': int(round(b['h'])), 'depth': b['depth']}
                          for b in lay['boxes']],
                'links': [{'from': l['from'], 'to': l['to'], 'cable': l['cable'],
                           'x1': int(round(l['x1'])), 'y1': int(round(l['y1'])),
                           'x2': int(round(l['x2'])), 'y2': int(round(l['y2'])),
                           'vertical': l.get('vertical', 0), 'dashed': l.get('dashed', 0),
                           'magenta': l.get('magenta', 0),
                           'bezier': list(l['bezier'])} for l in lay['links']],
            },
        }

    def scene_json(self, nodes, horizontal=False, title=''):
        import json as _json
        return _json.dumps(self.scene(nodes, horizontal, title), ensureAscii=False)

    @staticmethod
    def sample_bezier(b, steps):
        pts = []
        p0x, p0y, c1x, c1y, c2x, c2y, p3x, p3y = b[:8]
        for i in range(0, steps + 1):
            t = i / steps
            mt = 1 - t
            k0, k1, k2, k3 = mt * mt * mt, 3 * mt * mt * t, 3 * mt * t * t, t * t * t
            pts.append((k0 * p0x + k1 * c1x + k2 * c2x + k3 * p3x,
                        k0 * p0y + k1 * c1y + k2 * c2y + k3 * p3y))
        return pts

    @staticmethod
    def _seg_cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    def _seg_proper_intersect(self, a, b, c, d):
        eps = 1e-9
        d1 = self._seg_cross(c, d, a)
        d2 = self._seg_cross(c, d, b)
        d3 = self._seg_cross(a, b, c)
        d4 = self._seg_cross(a, b, d)
        s12 = (d1 > eps and d2 < -eps) or (d1 < -eps and d2 > eps)
        s34 = (d3 > eps and d4 < -eps) or (d3 < -eps and d4 > eps)
        return s12 and s34

    def countCrossings(self, links, steps=None):
        """HTML countCrossings：贝塞尔采样后判"真穿过"，相邻线段对逐个算。
        连线数量很大时（整图几千条）逐对求交没有意义也跑不完，直接跳过：
        状态栏那个"交叉数"只在小图上作参考。"""
        if len(links) > 600:
            return 0
        steps = steps or self.rules.cfg.get('curveSteps', 16)
        packs = []
        for l in links:
            pts = self.sample_bezier(l['bezier'], steps)
            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            packs.append({'pts': pts, 'x0': min(xs), 'x1': max(xs), 'y0': min(ys), 'y1': max(ys)})
        n = 0
        # 布局阶段几千条线时，朴素两两比较是 O(n²)（8000 条 = 3200 万对，几十秒）。
        # 先用均匀网格按包围盒分桶，只比较"落在同一格"的线对，结果不变。
        if len(packs) > 120:
            span_x = max([p['x1'] for p in packs] + [0]) - min([p['x0'] for p in packs] + [0])
            span_y = max([p['y1'] for p in packs] + [0]) - min([p['y0'] for p in packs] + [0])
            ew = sum(p['x1'] - p['x0'] for p in packs) / len(packs)
            eh = sum(p['y1'] - p['y0'] for p in packs) / len(packs)
            cell = max(24.0, (ew + eh) / 2.0)
            cols = max(1, int(span_x / cell) + 2)
            gx0 = min([p['x0'] for p in packs] + [0])
            gy0 = min([p['y0'] for p in packs] + [0])
            cells = {}
            for i, A in enumerate(packs):
                c0 = int((A['x0'] - gx0) / cell)
                c1 = int((A['x1'] - gx0) / cell)
                r0 = int((A['y0'] - gy0) / cell)
                r1 = int((A['y1'] - gy0) / cell)
                for r in range(r0, r1 + 1):
                    for c in range(c0, c1 + 1):
                        cells.setdefault(r * cols + c, []).append(i)
            seen = set()
            for bucket in cells.values():
                m = len(bucket)
                if m < 2:
                    continue
                for ii in range(m):
                    for jj in range(ii + 1, m):
                        i, j = bucket[ii], bucket[jj]
                        key = i * len(packs) + j
                        if key in seen:
                            continue
                        seen.add(key)
                        A, B = packs[i], packs[j]
                        if A['x1'] < B['x0'] or B['x1'] < A['x0'] or A['y1'] < B['y0'] or B['y1'] < A['y0']:
                            continue
                        pa, pb = A['pts'], B['pts']
                        for p in range(len(pa) - 1):
                            for q in range(len(pb) - 1):
                                if self._seg_proper_intersect(pa[p], pa[p + 1], pb[q], pb[q + 1]):
                                    n += 1
            return n
        for i in range(len(packs)):
            for j in range(i + 1, len(packs)):
                A, B = packs[i], packs[j]
                if A['x1'] < B['x0'] or B['x1'] < A['x0'] or A['y1'] < B['y0'] or B['y1'] < A['y0']:
                    continue
                pa, pb = A['pts'], B['pts']
                for p in range(len(pa) - 1):
                    for q in range(len(pb) - 1):
                        if self._seg_proper_intersect(pa[p], pa[p + 1], pb[q], pb[q + 1]):
                            n += 1
        return n

    def minimap_rects(self, layout_w, layout_h, zoom, ox, oy, view_w=None, view_h=None):
        mi = self.rules.mini
        w, h, pad = mi.get('w', 180), mi.get('h', 126), mi.get('pad', 4)
        s = min((w - pad * 2) / max(1, layout_w), (h - pad * 2) / max(1, layout_h))
        return {'w': w, 'h': h, 'pad': pad, 's': s,
                'bg': mi.get('bg', '#FBFCFE'), 'border': mi.get('border', '#E4E7EC'),
                'node': mi.get('node', '#CFE0F3'), 'viewport': mi.get('viewport', '#E15759'),
                'view_w': view_w, 'view_h': view_h, 'zoom': zoom, 'ox': ox, 'oy': oy}
