# -*- coding: utf-8 -*-
"""把"实现认、声明没写"的别名回填到 ownProperties（XamlProperty.aliases 字段）。

每个条目形如：
    {L"Content", XamlPropType::String, true, true}
    {L"SelectionMode", XamlPropType::Enum, true, true, L"Single|Multi"}
补齐后：
    {L"Content", XamlPropType::String, true, true, nullptr, L"Text"}
"""
import os
import re

CTRL = r'G:\PyArx\MfcXamlCore\controls'

# 文件 -> {属性主名: 别名（'|' 分隔）}
ALIASES = {
    'XamlBitmapButton.cpp': {'Content': 'Text', 'Image': 'Source|Src'},
    'XamlButton.cpp': {'Content': 'Text'},
    'XamlCheckBox.cpp': {'Content': 'Text'},
    'XamlCodeEditor.cpp': {'Content': 'Text', 'IsReadOnly': 'ReadOnly'},
    'XamlGridView.cpp': {'SelectedIndex': 'Index'},
    'XamlGroupBox.cpp': {'Content': 'Header|Text'},
    'XamlHyperlink.cpp': {'Text': 'Content', 'Url': 'Href|NavigateUrl'},
    'XamlListView.cpp': {'SelectedIndex': 'Index'},
    'XamlMenu.cpp': {'SelectedIndex': 'Index'},
    'XamlMenuBar.cpp': {'SelectedIndex': 'Index'},
    'XamlPageIndicator.cpp': {'CurrentIndex': 'Index'},
    'XamlPathView.cpp': {'CurrentIndex': 'Index'},
    'XamlRadioButton.cpp': {'Content': 'Text'},
    'XamlRichText.cpp': {'Text': 'Content'},
    'XamlScrolledWindow.cpp': {'ScrollMin': 'Minimum', 'ScrollMax': 'Maximum'},
    'XamlStaticBitmap.cpp': {'Image': 'Source|Src'},
    'XamlSwitch.cpp': {'Checked': 'IsChecked|On'},
    'XamlTextBlock.cpp': {'Content': 'Text|Header'},
    'XamlToggleButton.cpp': {'Content': 'Text'},
    'XamlToolBar.cpp': {'SelectedIndex': 'Index|CurrentIndex'},
    'XamlTreeView.cpp': {'SelectedRow': 'Index|CurrentIndex'},
}

# 同一个东西被声明成两个属性的重复条目：删掉后者
DROP_DUPLICATE = {
    'XamlCodeEditor.cpp': ['Text'],
    'XamlPageView.cpp': ['Index'],
}


def read(p):
    for enc in ('utf-8-sig', 'utf-8', 'gbk'):
        try:
            return open(p, encoding=enc).read(), enc
        except UnicodeDecodeError:
            continue
    raise SystemExit('decode failed: ' + p)


def entry_span(txt, prop):
    """返回 ownProperties 块里 {L"prop" ...} 这一条的 (start, end, field_count)。"""
    head = txt.find('ownProperties')
    if head < 0:
        return None
    tail = txt.find('};', head)
    block = txt[head:tail]
    m = re.search(r'\{L"%s"' % re.escape(prop), block)
    if not m:
        return None
    i = m.start()
    depth = 0
    j = i
    while j < len(block):
        c = block[j]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                break
        j += 1
    body = block[i:j + 1]
    inner = body[1:-1]
    # 顶层逗号数 = 字段数-1
    depth = 0
    commas = 0
    k = 0
    while k < len(inner):
        c = inner[k]
        if c in '({':
            depth += 1
        elif c in ')}':
            depth -= 1
        elif c == ',' and depth == 0:
            commas += 1
        k += 1
    return head + i, head + j + 1, commas + 1


def main():
    changed = 0
    for fn, mapping in ALIASES.items():
        p = os.path.join(CTRL, fn)
        txt, enc = read(p)
        for prop, alias in mapping.items():
            span = entry_span(txt, prop)
            if span is None:
                print('!! %s 里找不到属性 %s' % (fn, prop))
                continue
            a, b, fields = span
            body = txt[a:b]
            if fields == 4:
                body = body[:-1] + ', nullptr, L"%s"}' % alias
            elif fields == 5:
                body = body[:-1] + ', L"%s"}' % alias
            elif fields == 6:
                body = re.sub(r',\s*L"[^"]*"\}$', ', L"%s"}' % alias, body)
            else:
                print('!! %s.%s 字段数 %d 不认识' % (fn, prop, fields))
                continue
            txt = txt[:a] + body + txt[b:]
            changed += 1
        for prop in DROP_DUPLICATE.get(fn, []):
            span = entry_span(txt, prop)
            if span is None:
                print('!! %s 里找不到重复条目 %s' % (fn, prop))
                continue
            a, b, _ = span
            # 连同前面的空白和一个逗号一起删
            s = a
            while s > 0 and txt[s - 1] in ' \t':
                s -= 1
            e = b
            while e < len(txt) and txt[e] in ' \t,\r':
                e += 1
            txt = txt[:s] + txt[e:]
            changed += 1
        open(p, 'w', encoding='utf-8', newline='\n').write(txt)
    print('改动条目:', changed)


if __name__ == '__main__':
    main()
