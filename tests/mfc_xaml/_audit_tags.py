# -*- coding: utf-8 -*-
"""统计注册标签（含别名）在仓库 XAML/Python 里的真实使用量，用于决定保留哪个名字。"""
import os
import re
from collections import Counter

ROOT = r'G:\PyArx'
SKIP = ('\\X64\\', '\\third_party\\', '\\docs\\', '\\.git\\', '\\pylibs\\')
REG = os.path.join(ROOT, r'MfcXamlCore\controls\RegisterPyMfcControls.cpp')

# 解析 RegisterOne(..., {names})
src = open(REG, encoding='utf-8', errors='surrogateescape').read()
groups = []
for m in re.finditer(r'RegisterOne\(.*?\{\s*(.*?)\}\s*\)\s*;', src, re.S):
    names = re.findall(r'L"([^"]+)"', m.group(1))
    if names:
        groups.append(names)

usage = Counter()
for base, dirs, files in os.walk(ROOT):
    for fn in files:
        if not fn.lower().endswith(('.xaml', '.py')):
            continue
        p = os.path.join(base, fn)
        if any(s in p for s in SKIP):
            continue
        txt = open(p, encoding='utf-8', errors='surrogateescape').read()
        for tag in re.findall(r'<([A-Za-z_][A-Za-z0-9_.:]*)\b', txt):
            usage[tag.lower()] += 1

print('组数', len(groups))
for names in groups:
    primary = names[0]
    row = []
    for n in names:
        c = usage.get(n.lower(), 0)
        row.append('%s=%d' % (n, c))
    print('%-16s %s' % (primary, '  '.join(row)))

print()
print('仓库里出现但未注册的标签（前 40）:')
known = {n.lower() for names in groups for n in names}
others = [(t, c) for t, c in usage.items() if t not in known]
others.sort(key=lambda x: -x[1])
print(', '.join('%s=%d' % x for x in others[:40]))
