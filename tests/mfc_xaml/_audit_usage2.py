# -*- coding: utf-8 -*-
"""按控件标签统计属性写法：分别是哪个标签用了 Content / Text / Checked / IsChecked 等。"""
import os
import re
from collections import Counter

ROOT = r'G:\PyArx'
SKIP = ('X64', 'x64', 'obj', 'third_party', 'packages', '.git', 'log', 'docs')
ATTRS = ['Content', 'Text', 'Header', 'Checked', 'IsChecked', 'Image', 'Source', 'Src',
         'Url', 'Href', 'Index', 'SelectedIndex', 'CurrentIndex', 'SelectedRow',
         'Minimum', 'Min', 'Maximum', 'Max', 'Position', 'Page', 'PageSize',
         'From', 'To', 'LowValue', 'HighValue', 'ModelCount', 'Count', 'ReadOnly', 'IsReadOnly']

pairs = Counter()
for dirpath, dirnames, filenames in os.walk(ROOT):
    if any(s in dirpath for s in SKIP):
        dirnames[:] = []
        continue
    dirnames[:] = [d for d in dirnames if d not in SKIP]
    for fn in filenames:
        if not fn.lower().endswith(('.xaml', '.py')):
            continue
        p = os.path.join(dirpath, fn)
        try:
            txt = open(p, encoding='utf-8', errors='ignore').read()
        except OSError:
            continue
        for m in re.finditer(r'<([A-Za-z][A-Za-z0-9_.]*)\b([^<>]*)>', txt):
            tag, body = m.group(1), m.group(2)
            for a in ATTRS:
                if re.search(r'\b%s\s*=' % a, body):
                    pairs[(tag, a)] += 1

for (tag, a), n in sorted(pairs.items(), key=lambda kv: (-kv[1], kv[0][0], kv[0][1])):
    if n >= 2:
        print('%-16s %-14s %3d' % (tag, a, n))
