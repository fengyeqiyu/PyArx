# -*- coding: utf-8 -*-
"""统计仓库里 XAML/代码中各种属性写法的实际用量，用来定"规范名"。"""
import os
import re
from collections import Counter

ROOT = r'G:\PyArx'
SKIP = ('X64', 'x64', 'Debug', 'Release', 'obj', 'third_party', 'packages', '.git', 'log')
PATTERNS = [
    ('Content', r'\bContent\s*='),
    ('Text', r'\bText\s*='),
    ('Header', r'\bHeader\s*='),
    ('Caption', r'\bCaption\s*='),
    ('Image', r'\bImage\s*='),
    ('Source', r'\bSource\s*='),
    ('Src', r'\bSrc\s*='),
    ('Url', r'\bUrl\s*='),
    ('Href', r'\bHref\s*='),
    ('NavigateUrl', r'\bNavigateUrl\s*='),
    ('Index', r'\bIndex\s*='),
    ('SelectedIndex', r'\bSelectedIndex\s*='),
    ('CurrentIndex', r'\bCurrentIndex\s*='),
    ('SelectedRow', r'\bSelectedRow\s*='),
    ('ItemCount', r'\bItemCount\s*='),
    ('Checked', r'\bChecked\s*='),
    ('IsChecked', r'\bIsChecked\s*='),
    ('On', r'\bOn\s*='),
    ('Minimum', r'\bMinimum\s*='),
    ('Min', r'\bMin\s*='),
    ('Maximum', r'\bMaximum\s*='),
    ('Max', r'\bMax\s*='),
    ('Value', r'\bValue\s*='),
    ('Position', r'\bPosition\s*='),
    ('PageSize', r'\bPageSize\s*='),
    ('Page', r'\bPage\s*='),
    ('LowValue', r'\bLowValue\s*='),
    ('HighValue', r'\bHighValue\s*='),
    ('From', r'\bFrom\s*='),
    ('To', r'\bTo\s*='),
    ('ReadOnly', r'\bReadOnly\s*='),
    ('IsReadOnly', r'\bIsReadOnly\s*='),
    ('Items', r'\bItems\s*='),
    ('Strings', r'\bStrings\s*='),
    ('Choices', r'\bChoices\s*='),
    ('CurSel', r'\bCurSel\s*='),
    ('Orientation', r'\bOrientation\s*='),
    ('ScrollMin', r'\bScrollMin\s*='),
    ('ScrollMax', r'\bScrollMax\s*='),
    ('Count', r'\bCount\s*='),
    ('ModelCount', r'\bModelCount\s*='),
]

cnt = Counter()
files = Counter()
for dirpath, dirnames, filenames in os.walk(ROOT):
    if any(s in dirpath for s in SKIP):
        dirnames[:] = []
        continue
    dirnames[:] = [d for d in dirnames if d not in SKIP]
    for fn in filenames:
        if not fn.lower().endswith(('.xaml', '.py', '.html', '.json', '.xml', '.js')):
            continue
        p = os.path.join(dirpath, fn)
        try:
            txt = open(p, encoding='utf-8', errors='ignore').read()
        except OSError:
            continue
        for name, pat in PATTERNS:
            n = len(re.findall(pat, txt))
            if n:
                cnt[name] += n
                files[name] += 1
for name, _ in PATTERNS:
    print('%-14s 出现 %4d 次，涉及 %3d 个文件' % (name, cnt[name], files[name]))
