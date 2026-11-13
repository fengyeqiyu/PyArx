# -*- coding: utf-8 -*-
"""属性命名统一：去掉别名分支，一个属性只留一个规范名。

规范名（小写，代码里的 key）：text / header / checked / value / minimum / maximum /
lowvalue / highvalue / selectedindex / pagesize / items / image / url / isreadonly /
scrollmin / scrollmax / count / orientation / columns / rows / syntax / nodes / pages /
title / lines / content(Popup 专用) / data / commands / script / cellwidth / cellheight /
itemheight
"""
import os
import re

CTRL = r'G:\PyArx\MfcXmlCore\controls'

FILE_MAP = {
    'XmlButton.cpp': {'content': 'text'},
    'XmlBitmapButton.cpp': {'content': 'text', 'source': 'image', 'src': 'image'},
    'XmlCheckBox.cpp': {'content': 'text', 'ischecked': 'checked'},
    'XmlRadioButton.cpp': {'content': 'text', 'ischecked': 'checked'},
    'XmlToggleButton.cpp': {'content': 'text'},
    'XmlTextBlock.cpp': {'content': 'text', 'header': 'text'},
    'XmlGroupBox.cpp': {'content': 'header', 'text': 'header'},
    'XmlCodeEditor.cpp': {'content': 'text', 'readonly': 'isreadonly'},
    'XmlHyperlink.cpp': {'content': 'text', 'href': 'url', 'navigateurl': 'url'},
    'XmlRichText.cpp': {'content': 'text'},
    'XmlToolTip.cpp': {'content': 'text'},
    'XmlGridView.cpp': {'index': 'selectedindex'},
    'XmlListView.cpp': {'index': 'selectedindex'},
    'XmlMenu.cpp': {'index': 'selectedindex'},
    'XmlMenuBar.cpp': {'index': 'selectedindex'},
    'XmlPathView.cpp': {'index': 'selectedindex'},
    'XmlPageIndicator.cpp': {'index': 'selectedindex'},
    'XmlToolBar.cpp': {'index': 'selectedindex', 'currentindex': 'selectedindex'},
    'XmlTreeView.cpp': {'index': 'selectedindex', 'currentindex': 'selectedindex'},
    'XmlPageView.cpp': {'index': 'selectedindex', 'currentindex': 'selectedindex'},
    'XmlSwitch.cpp': {'ischecked': 'checked', 'on': 'checked'},
    'XmlScrolledWindow.cpp': {'minimum': 'scrollmin', 'maximum': 'scrollmax'},
    'XmlStaticBitmap.cpp': {'source': 'image', 'src': 'image'},
    # 属性表控件
    'XmlSlider.cpp': {'min': 'minimum', 'max': 'maximum', 'position': 'value'},
    'XmlProgressBar.cpp': {'min': 'minimum', 'max': 'maximum', 'position': 'value'},
    'XmlSpinBox.cpp': {'min': 'minimum', 'max': 'maximum', 'position': 'value'},
    'XmlDial.cpp': {'from': 'minimum', 'to': 'maximum', 'position': 'value',
                     'min': 'minimum', 'max': 'maximum'},
    'XmlRangeSlider.cpp': {'from': 'lowvalue', 'to': 'highvalue',
                            'min': 'minimum', 'max': 'maximum'},
    'XmlScrollBar.cpp': {'position': 'value', 'page': 'pagesize'},
    'XmlTumbler.cpp': {'currentindex': 'selectedindex', 'index': 'selectedindex',
                        'value': 'selectedindex', 'modelcount': 'count'},
    'XmlTextBox.cpp': {'content': 'text', 'caption': 'text', 'readonly': 'isreadonly',
                        'hint': 'placeholder', 'cuebanner': 'placeholder'},
    'XmlComboBox.cpp': {'strings': 'items', 'choices': 'items', 'content': 'text',
                         'cursel': 'selectedindex'},
    'XmlTableView.cpp': {'currentindex': 'selectedindex', 'index': 'selectedindex',
                          'selectedrow': 'selectedindex'},
}

# 声明名（显示用）也要跟着统一
DECL_MAP = {
    'Content': 'Text', 'Caption': 'Text', 'Header': 'Header', 'IsChecked': 'Checked',
    'CurrentIndex': 'SelectedIndex', 'Index': 'SelectedIndex', 'SelectedRow': 'SelectedIndex',
    'ReadOnly': 'IsReadOnly', 'Min': 'Minimum', 'Max': 'Maximum', 'Position': 'Value',
    'Page': 'PageSize', 'Strings': 'Items', 'Choices': 'Items', 'CurSel': 'SelectedIndex',
    'ModelCount': 'Count', 'From': 'LowValue', 'To': 'HighValue',
}


def read(p):
    for enc in ('utf-8-sig', 'utf-8', 'gbk'):
        try:
            return open(p, encoding=enc).read()
        except UnicodeDecodeError:
            continue
    raise SystemExit('decode failed: ' + p)


CHAIN = re.compile(r'xmlprop::StrEq\(key,\s*"([a-z0-9_.]+)"\)'
                   r'(?:\s*\|\|\s*xmlprop::StrEq\(key,\s*"[a-z0-9_.]+"\))+')
SINGLE = re.compile(r'xmlprop::StrEq\(key,\s*"([a-z0-9_.]+)"\)')


def canon(tokens, mapping):
    out = []
    for t in tokens:
        c = mapping.get(t, t)
        if c not in out:
            out.append(c)
    return out[0] if len(out) == 1 else None, out


def main():
    report = []
    for fn in sorted(os.listdir(CTRL)):
        if not fn.endswith('.cpp') or not fn.startswith('Xml'):
            continue
        mapping = FILE_MAP.get(fn, {})
        p = os.path.join(CTRL, fn)
        txt = read(p)
        if 'StrEq' not in txt:
            continue

        def repl_chain(m):
            tokens = re.findall(r'StrEq\(key,\s*"([a-z0-9_.]+)"', m.group(0))
            one, many = canon(tokens, mapping)
            if one is None:
                report.append('%s 链里有多个规范名 %s -> %s' % (fn, tokens, many))
                one = many[0]
            return 'xmlprop::StrEq(key, "%s")' % one

        txt2 = CHAIN.sub(repl_chain, txt)
        # 单条也统一（只改有映射的）
        def repl_single(m):
            t = m.group(1)
            return 'xmlprop::StrEq(key, "%s")' % mapping.get(t, t)

        txt2 = SINGLE.sub(repl_single, txt2)

        # 声明名统一（Popup 的 Content 是"正文"，不是文本，保持不动）
        def repl_decl(m):
            name = m.group(1)
            if fn in ('XmlPopup.cpp',):
                return '{L"%s"' % name
            return '{L"%s"' % DECL_MAP.get(name, name)

        txt2 = re.sub(r'\{L"([A-Za-z][A-Za-z0-9_.]*)"', repl_decl, txt2)
        # 属性表第二参数（别名）一律清空
        txt2 = re.sub(r'\{L"([^"]+)", L"[^"]*", XmlPropType::',
                      r'{L"\1", nullptr, XmlPropType::', txt2)

        if txt2 != txt:
            open(p, 'w', encoding='utf-8', newline='\n').write(txt2)
    for r in report:
        print('!', r)


if __name__ == '__main__':
    main()
