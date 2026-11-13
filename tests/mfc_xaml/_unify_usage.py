# -*- coding: utf-8 -*-
"""把仓库里的 XAML 用法同步到规范属性名（与 _unify_props.py 的规范名一致）。"""
import os
import re

ROOT = r'G:\PyArx'
SKIP = ('\\X64\\', '\\third_party\\', '\\docs\\', '\\.git\\', '\\pylibs\\')
EXT = ('.xaml', '.py')

TEXT_TAGS = {'button', 'checkbox', 'radiobutton', 'togglebutton', 'bitmapbutton',
             'textblock', 'codeeditor', 'hyperlink', 'richtext', 'tooltip', 'label'}


def tag_map(tag):
    t = tag.lower()
    m = {}
    if t in TEXT_TAGS:
        m['Content'] = 'Text'
    if t == 'groupbox':
        m['Content'] = 'Header'
        m['Text'] = 'Header'
    if t in ('checkbox', 'radiobutton', 'switch', 'togglebutton'):
        m['IsChecked'] = 'Checked'
    if t in ('listview', 'gridview', 'treeview', 'pathview', 'pageindicator',
             'pageview', 'toolbar', 'menu', 'menubar', 'combobox', 'tabcontrol'):
        m['Index'] = 'SelectedIndex'
        m['CurrentIndex'] = 'SelectedIndex'
        m['SelectedRow'] = 'SelectedIndex'
        m['CurSel'] = 'SelectedIndex'
    if t == 'hyperlink':
        m['Href'] = 'Url'
        m['NavigateUrl'] = 'Url'
    if t in ('staticbitmap', 'bitmapbutton'):
        m['Source'] = 'Image'
        m['Src'] = 'Image'
    if t in ('textbox', 'passwordbox', 'codeeditor'):
        m['ReadOnly'] = 'IsReadOnly'
    if t in ('combobox', 'listview', 'pathview'):
        m['Strings'] = 'Items'
        m['Choices'] = 'Items'
    if t in ('slider', 'progressbar', 'spinbox', 'dial'):
        m['Position'] = 'Value'
        m['Min'] = 'Minimum'
        m['Max'] = 'Maximum'
        m['From'] = 'Minimum'
        m['To'] = 'Maximum'
    if t == 'rangeslider':
        m['From'] = 'LowValue'
        m['To'] = 'HighValue'
    if t == 'scrollbar':
        m['Page'] = 'PageSize'
        m['Position'] = 'Value'
    if t == 'tumbler':
        m['ModelCount'] = 'Count'
        m['Value'] = 'SelectedIndex'
        m['Index'] = 'SelectedIndex'
        m['CurrentIndex'] = 'SelectedIndex'
    if t == 'scrolledwindow':
        m['Minimum'] = 'ScrollMin'
        m['Maximum'] = 'ScrollMax'
    return m


TAG = re.compile(r'<([A-Za-z_][A-Za-z0-9_.:]*)\b([^<>]*?)(/?)>')


def fix_attrs(tag, attrs):
    m = tag_map(tag)
    if not m:
        return attrs, 0
    n = 0
    for old, new in m.items():
        pat = re.compile(r'(\s)' + old + r'(\s*=)')
        attrs, k = pat.subn(r'\g<1>' + new + r'\2', attrs)
        n += k
    return attrs, n


def process(path):
    raw = open(path, encoding='utf-8', errors='surrogateescape').read()
    total = [0]

    def repl(m):
        tag, attrs, close = m.group(1), m.group(2), m.group(3)
        new_attrs, n = fix_attrs(tag, attrs)
        total[0] += n
        return '<%s%s%s>' % (tag, new_attrs, close)

    out = TAG.sub(repl, raw)
    if out != raw:
        open(path, 'w', encoding='utf-8', newline='').write(out)
    return total[0]


def main():
    changed = []
    for base, dirs, files in os.walk(ROOT):
        for fn in files:
            if not fn.lower().endswith(EXT):
                continue
            p = os.path.join(base, fn)
            if any(s in p for s in SKIP):
                continue
            n = process(p)
            if n:
                changed.append((p, n))
    for p, n in changed:
        print('%3d  %s' % (n, p.replace(ROOT + '\\', '')))
    print('files changed:', len(changed), 'attrs:', sum(n for _, n in changed))


if __name__ == '__main__':
    main()
