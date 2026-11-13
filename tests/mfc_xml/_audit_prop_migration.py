# -*- coding: utf-8 -*-
"""统计每个控件遗留（ownProperties 三钩子）属性数量，用于分批迁移到属性表。"""
import os
import re

CTRL = r'G:\PyArx\MfcXmlCore\controls'
out = []
for fn in sorted(os.listdir(CTRL)):
    if not (fn.startswith('Xml') and fn.endswith('.cpp')):
        continue
    t = open(os.path.join(CTRL, fn), encoding='utf-8-sig', errors='replace').read()
    if 'ownProperties(' not in t or 'ownPropTable' in t:
        continue
    props = re.findall(r'\{L"([A-Za-z]+)",\s*XmlPropType::([A-Za-z]+)', t)
    out.append((len(props), fn, ','.join(n for n, _ in props)))
out.sort(reverse=True)
for n, fn, names in out:
    print('%-2d %-28s %s' % (n, fn, names))
print('共 %d 个控件待迁移，属性合计 %d 项' % (len(out), sum(o[0] for o in out)))
