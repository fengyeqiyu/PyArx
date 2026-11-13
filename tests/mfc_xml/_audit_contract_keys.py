# -*- coding: utf-8 -*-
"""校验：每个控件的契约名（type()）都在事件契约表里有键，且表里没有多余键。"""
import os
import re

CTRL = r'G:\PyArx\MfcXmlCore\controls'
ev = open(os.path.join(CTRL, 'XmlEvents.h'), encoding='utf-8', errors='surrogateescape').read()
body = ev[ev.index('kTable = {'):ev.index('};', ev.index('kTable = {'))]
keys = set(re.findall(r'\{L"([a-z0-9_]+)"', body))

types = {}
for fn in os.listdir(CTRL):
    if not fn.endswith('.h'):
        continue
    txt = open(os.path.join(CTRL, fn), encoding='utf-8', errors='surrogateescape').read()
    for m in re.finditer(r'XmlMfcControlBase\(L"([^"]*)"\)', txt):
        types[fn] = m.group(1).lower()
    if 'XmlPageView' in fn and fn not in types:
        types[fn] = 'stackview/swipeview/page'

print('控件数', len(types), ' 表键数', len(keys))
missing = {f: t for f, t in types.items() if t not in keys and '/' not in t}
extra = sorted(keys - set(types.values()))
print('控件类型不在表里:', missing if missing else '无')
print('表里没有对应控件:', extra if extra else '无')
