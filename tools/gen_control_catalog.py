# -*- coding: utf-8 -*-
r"""按 prop_schema() 补齐 UIDesign 的控件目录 control_catalog.xml。

用法：python.exe G:\PyArx\tools\gen_control_catalog.py
规则：
  · 已有 <objectinfo> 原样保留（分类、显示名、默认尺寸、属性表都不动）；
  · schema 里有、目录里没有的主标签，按 CATEGORY / SIZE 追加一条，属性由 schema 生成；
  · 不写别名，只写主标签。
"""
import json
import os
import re
import sys

ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
CATALOG = os.path.join(ROOT, 'UIDesign', 'control_catalog.xml')

sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
import PyMfcXml as M

# 新增控件放哪一栏
CATEGORY = {
    'CodeEditor': 'Input Widgets',
    'SpinBox': 'Input Widgets',
    'RangeSlider': 'Input Widgets',
    'Switch': 'Input Widgets',
    'Dial': 'Input Widgets',
    'Tumbler': 'Input Widgets',
    'ScrollBar': 'Input Widgets',
    'Slider': 'Input Widgets',
    'Function Bar': 'Function Bar',
    'ToolBar': 'Function Bar',
    'MenuBar': 'Function Bar',
    'Menu': 'Overlay',
    'ToolTip': 'Overlay',
    'Popup': 'Overlay',
    'TableView': 'Data Views',
    'TreeTable': 'Data Views',
    'VirtualList': 'Data Views',
    'GridView': 'Data Views',
    'PathView': 'Data Views',
    'PageIndicator': 'Data Views',
    'TabBar': 'Data Views',
    'Page': 'Data Views',
    'StackView': 'Data Views',
    'SwipeView': 'Data Views',
    'PowerCanvas': 'Canvas',
    'QCanvas': 'Canvas',
    'Flickable': 'Containers',
    'BusyIndicator': 'Display Widgets',
    'MonthGrid': 'Input Widgets',
    'RangeSlider_': 'Input Widgets',
}
# 别名条目清掉：目录只留主标签（TrackBar 是 Slider 的历史别名）
ALIAS_DROP = {'TrackBar'}
CAT_ORDER = ['Buttons', 'Input Widgets', 'Display Widgets', 'Data Views', 'Containers',
             'Function Bar', 'Overlay', 'Canvas']

SIZE = {
    'Table': (320, 140), 'TreeTable': (320, 140), 'VirtualList': (240, 140),
    'GridView': (320, 160), 'PathView': (320, 140), 'PowerCanvas': (420, 260),
    'QCanvas': (420, 200), 'Flickable': (320, 160), 'TabBar': (360, 160),
    'Page': (360, 160), 'StackView': (360, 160), 'SwipeView': (360, 160),
    'BusyIndicator': (56, 56), 'MonthGrid': (260, 200), 'PageIndicator': (160, 22),
    'MenuBar': (360, 28), 'ToolBar': (360, 32), 'ToolTip': (160, 24),
    'Popup': (320, 160), 'Menu': (200, 120), 'SpinBox': (120, 30),
    'RangeSlider': (200, 34), 'Switch': (64, 28), 'Dial': (88, 88),
    'Tumbler': (88, 96), 'ScrollBar': (200, 18), 'Slider': (200, 34),
    'CodeEditor': (360, 160),
}

TYPE_MAP = {
    'Enum': 'enum', 'Int': 'int', 'Float': 'float', 'Color': 'color', 'Bool': 'bool',
    'String': 'text', 'Text': 'text',
}


def esc(s):
    return (s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
            .replace('"', '&quot;'))


def prop_xml(prop):
    kind = TYPE_MAP.get(prop.get('type'), 'text')
    name = prop['name']
    if kind == 'enum':
        opts = [o for o in str(prop.get('enum', '')).split('|') if o]
        if not opts:
            kind = 'text'
        else:
            return f'    <property name="{name}" type="enum" options="{esc("|".join(opts))}"/>'
    return f'    <property name="{name}" type="{kind}"/>'


def main():
    with open(CATALOG, 'r', encoding='utf-8') as fh:
        text = fh.read()

    head = text.split('<package', 1)[0]
    pkg_line = '<package' + text.split('<package', 1)[1].split('>', 1)[0] + '>'
    blocks = re.findall(r'  <objectinfo .*?</objectinfo>', text, re.S)
    known = {}
    for block in blocks:
        tag = re.search(r'<objectinfo tag="([^"]+)"', block).group(1)
        known[tag] = block

    for tag in list(known):
        if tag in ALIAS_DROP:
            del known[tag]

    schema = json.loads(M.prop_schema())['controls']
    missing = [c for c in schema if c['tag'] not in known]

    for ctrl in missing:
        tag = ctrl['tag']
        cat = CATEGORY.get(tag, 'Display Widgets')
        w, h = SIZE.get(tag, (160, 32))
        body = [f'  <objectinfo tag="{tag}" display="{tag}" category="{cat}" w="{w}" h="{h}">']
        body.append('    <property name="Name" type="text"/>')
        for prop in ctrl['properties']:
            if prop['name'] in ('Level', 'TabIndex', 'Theme'):
                continue
            body.append(prop_xml(prop))
        body.append('  </objectinfo>')
        known[tag] = '\n'.join(body)

    # 按分类顺序输出，同分类内保持原文件顺序
    def cat_of(block):
        m = re.search(r'category="([^"]+)"', block)
        return m.group(1) if m else 'Display Widgets'

    ordered = []
    for cat in CAT_ORDER:
        ordered += [known[t] for t in known if cat_of(known[t]) == cat]
    for t in known:                      # 兜底：未知分类追加到末尾
        if cat_of(known[t]) not in CAT_ORDER:
            ordered.append(known[t])

    out = head + pkg_line + '\n\n' + '\n\n'.join(ordered) + '\n\n</package>\n'
    with open(CATALOG, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write(out)

    print(f'目录控件 {len(known)} 个（新增 {len(missing)}）：{", ".join(c["tag"] for c in missing)}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
