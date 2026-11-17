# -*- coding: utf-8 -*-
"""在供电（母线）图纸里提取开关柜编号与用途，形成馈电关系。"""
import json

d = json.load(open(r'G:\PyArx\viewer\web\scene_pow.json', encoding='utf-8'))
tx = [(t[0], t[1], str(t[6]).strip()) for t in (d.get('tx') or [])]
keys = [('编号', '开关柜编号'), ('用途', '开关柜用途'), ('型号', '开关柜型号'),
        ('尺寸', '尺寸mm'), ('备注', '备  注')]
hits = [(x, y, s) for (x, y, s) in tx if '开关柜编号' in s or '开关柜用途' in s]
print('标签数', len(hits))
for x, y, s in hits[:40]:
    near = [(abs(x2 - x) + abs(y2 - y), x2, y2, s2) for (x2, y2, s2) in tx
            if abs(x2 - x) < 2600 and abs(y2 - y) < 900 and s2 != s]
    near.sort()
    print('%-10s (%.0f,%.0f)' % (s, x, y), [n[3] for n in near[:6]])
