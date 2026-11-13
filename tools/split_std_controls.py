# -*- coding: utf-8 -*-
"""把 header-only 的标准控件（XamlButton.h 这类）也拆成 .h + .cpp。

这些文件本来就"一个类一个文件"，缺的是把类体里的成员实现搬到 .cpp。
复用 split_extra_controls.py 的切分逻辑，保证和 41 个扩展控件同一套规则。
"""
import os
import re
import sys

sys.path.insert(0, r'G:\PyArx\tools')
from split_extra_controls import brace_body, split_members, qualify  # noqa: E402

ROOT = r'G:\PyArx\MfcXamlCore\controls'
TARGETS = ['XamlButton', 'XamlCheckBox', 'XamlCodeEditor', 'XamlComboBox', 'XamlGroupBox',
           'XamlListBox', 'XamlProgressBar', 'XamlRadioButton', 'XamlSlider',
           'XamlTextBlock', 'XamlTextBox']


def read(p):
    with open(p, 'r', encoding='utf-8-sig') as f:
        return f.read()


def write(p, t):
    with open(p, 'w', encoding='utf-8-sig', newline='\r\n') as f:
        f.write(t)


total = 0
for name in TARGETS:
    p = os.path.join(ROOT, name + '.h')
    text = read(p)
    m = re.search(r'^class\s+(\w+)\s*:\s*public\s+(\w+)', text, re.M)
    if not m:
        print('跳过（找不到类）:', name)
        continue
    parent = m.group(2)
    lb = text.find('{', m.end())
    body, _ = brace_body(text, lb)
    decl, moves = split_members(body, name)

    head_comment = text[:m.start()].rstrip()
    header = (head_comment + '\n\n'
              'class XAML_CORE_API ' + name + ' : public ' + parent + '\n{\n'
              + decl.rstrip() + '\n};\n')
    write(p, header)

    lines = ['// %s.cpp —— 实现（从 header-only 拆出，UTF-8 with BOM）' % name,
             '#include "%s.h"' % name, '']
    for def_sig, inner, _ in moves:
        lines.append(qualify(def_sig, name))
        lines.append('{' + inner + '}')
        lines.append('')
    write(os.path.join(ROOT, name + '.cpp'), '\n'.join(lines))
    total += len(moves)
    print('%-18s 函数 %d' % (name, len(moves)))

print('合计搬出 %d 个函数' % total)
