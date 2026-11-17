# -*- coding: utf-8 -*-
"""ElNode 单表核对页：所有柜一张表，柜与柜之间空一行。"""
import json

N = json.load(open(r'G:\PyArx\viewer\export\el_nodes.json', encoding='utf-8'))['nodes']

css = """
body{background:#0e1113;color:#dfe6ed;font:13px/1.6 'Microsoft YaHei',sans-serif;margin:18px}
h1{font-size:17px;color:#8fd3ff;margin:0 0 4px}
.sub{color:#7d8792;font-size:12px;margin-bottom:12px}
table{border-collapse:collapse;width:100%}
th{background:#1d262c;color:#9fb0bf;text-align:left;padding:5px 7px;border-bottom:1px solid #353e46;position:sticky;top:0}
td{padding:3px 7px;border-bottom:1px solid #1c2429;font-size:12px}
tr.cab td{background:#1a2228;color:#ffd479;font-weight:bold;padding:7px}
tr.cab td .n{color:#9fb0bf;font-weight:normal}
tr.gap td{height:18px;border:none;background:transparent}
"""

out = ['<!doctype html><meta charset="utf-8"><style>', css, '</style>',
       '<h1>ElNode 统计表</h1><div class="sub">%d 台｜出线 %d 回路｜柜间空行</div>' % (
           len(N), sum(len(n['outLines']) for n in N)),
       '<table><thead><tr><th>回路</th><th>开关</th><th>电缆</th><th>去向</th><th>功率</th></tr></thead><tbody>']

for n in N:
    inl = '；'.join('%s %s' % (x.get('cable', ''), x.get('breaker', '')) for x in n['inLines']) or '—'
    ctl = '；'.join('%s(%s)' % (c['name'], c['cable']) for c in (n.get('controls') or [])) or '—'
    sub = 'Pe=%s Ijs=%s <span class="n">｜ 进线 %d：%s ｜ 控制 %d：%s ｜ 备用 %d</span>' % (
        n.get('pe'), n.get('ijs'), len(n['inLines']), inl, len(n.get('controls') or []), ctl, len(n['spares']))
    out.append('<tr class="cab"><td colspan="5">%s　%s</td></tr>' % (n['id'], sub))
    for o in n['outLines']:
        out.append('<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>' % (
            o['id'], o['breaker'], o['cable'], o['target'], o['power']))
    out.append('<tr class="gap"><td colspan="5">&nbsp;</td></tr>')

out.append('</tbody></table>')
open(r'G:\PyArx\viewer\export\el_nodes.html', 'w', encoding='utf-8').write('\n'.join(out))
print('写出 el_nodes.html（单表）')
