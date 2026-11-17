# -*- coding: utf-8 -*-
"""生成配电回路台账（长表）：序号/起点/回路/开关/电缆/终点/功率，柜与柜之间空一行。"""
import json

N = json.load(open(r'G:\PyArx\viewer\export\el_nodes.json', encoding='utf-8'))['nodes']

rows = [["序号", "起点", "回路", "开关", "电缆", "终点", "功率"]]
idx = 0
for n in N:
    k = len(n['inLines']) or 1
    for j, il in enumerate(n['inLines'], 1):
        idx += 1
        code = 'inline%d' % j if k > 1 else 'inline1'
        rows.append([idx, n['id'], code, il.get('breaker') or '进线开关', il.get('cable', ''), il.get('from', ''), n.get('pe') or ''])
    for o in n['outLines']:
        idx += 1
        rows.append([idx, n['id'], o['id'], o['breaker'], o['cable'], o['target'], o['power']])
    rows.append(['', '', '', '', '', '', ''])  # 柜间空行

json.dump({'sheets': [{'name': '配电回路台账', 'columns': rows[0], 'header': True, 'mode': 'overwrite', 'data': rows[1:]}]},
          open(r'G:\PyArx\viewer\export\el_ledger.json', 'w', encoding='utf-8'), ensure_ascii=False)
json.dump(rows, open(r'G:\PyArx\viewer\export\el_ledger_values.json', 'w', encoding='utf-8'), ensure_ascii=False)
print('台账行数', len(rows), '（含表头与柜间空行）')
