# -*- coding: utf-8 -*-
"""生成控件"类名 / 中文名"对照表。

数据来源：
  UIDesign/control_catalog.xml          控件箱登记（tag / category / 默认尺寸）
  docs/mfcxaml-控件接口清单.md          实现方式 / 档位 / 源文件（由 prop_schema() 重算）
  docs/_schema.json                    属性条数（可选，gen_control_doc.py 的产物）

用法：python tools\\gen_control_names.py
输出：docs\\mfcxaml-控件中文名对照表.md
"""
import json
import os
import re
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CATALOG = os.path.join(ROOT, 'UIDesign', 'control_catalog.xml')
IFACE = os.path.join(ROOT, 'docs', 'mfcxaml-控件接口清单.md')
SCHEMA = os.path.join(ROOT, 'docs', '_schema.json')
OUT = os.path.join(ROOT, 'docs', 'mfcxaml-控件中文名对照表.md')

# tag -> 中文名（与设计器控件箱、文档统一用这一套，不再用别名）
CN = {
    'Button': '按钮',
    'CheckBox': '复选框',
    'RadioButton': '单选按钮',
    'ToggleButton': '开关按钮',
    'BitmapButton': '图标按钮',
    'TextBox': '文本框',
    'ComboBox': '下拉框',
    'ListBox': '列表框',
    'Choice': '选择框',
    'DatePicker': '日期选择框',
    'CodeEditor': '代码编辑器',
    'Slider': '滑块',
    'Switch': '开关',
    'SpinBox': '数值步进框',
    'RangeSlider': '区间滑块',
    'Dial': '旋钮',
    'Tumbler': '滚轮选择器',
    'ScrollBar': '滚动条',
    'TextBlock': '文本标签',
    'ProgressBar': '进度条',
    'Hyperlink': '超链接',
    'RichText': '富文本',
    'StaticBitmap': '图片',
    'CalendarCtrl': '日历',
    'BusyIndicator': '忙碌指示器',
    'PageIndicator': '页指示器',
    'TableView': '数据表格',
    'TreeTable': '树形表格',
    'VirtualList': '虚拟列表',
    'StackView': '栈式视图',
    'SwipeView': '滑动视图',
    'GridView': '网格视图',
    'PathView': '路径视图',
    'Page': '页面',
    'GroupBox': '分组框',
    'TabControl': '选项卡',
    'Border': '边框容器',
    'Panel': '面板容器',
    'Splitter': '分隔条',
    'TreeCtrl': '树形控件',
    'ListCtrl': '列表控件',
    'StaticLine': '分隔线',
    'GridCtrl': '网格控件',
    'ScrolledWindow': '滚动窗口',
    'Flickable': '拖拽视口',
    'ToolStrip': '工具栏',
    'MenuStrip': '菜单栏',
    'ToolTip': '提示气泡',
    'Menu': '弹出菜单',
    'Popup': '浮层',
    'PowerCanvas': '配电规划画布',
    'QCanvas': '通用画布',
    'Line': '直线',
    'Rectangle': '矩形',
    'Ellipse': '椭圆',
    'Arc': '圆弧',
}

# 分类 -> 中文分类（控件箱分组，顺序即控件箱顺序）
CAT_CN = {
    'Buttons': '按钮',
    'Input Widgets': '输入控件',
    'Display Widgets': '显示控件',
    'Data Views': '数据视图',
    'Containers': '容器',
    'Function Bar': '功能条',
    'Overlay': '浮层',
    'Canvas': '画布',
    'Draw': '绘制图元',
}
CAT_ORDER = ['Containers', 'Buttons', 'Input Widgets', 'Display Widgets',
             'Data Views', 'Function Bar', 'Overlay', 'Canvas', 'Draw']

ROW_RE = re.compile(r'\|\s*\*\*(?P<tag>[A-Za-z]+)\*\*\s*\|(?P<rest>.*)\|')


def read_catalog():
    """tag -> (category, w, h)"""
    out = {}
    root = ET.parse(CATALOG).getroot()
    for oi in root.findall('objectinfo'):
        out[oi.get('tag')] = (
            oi.get('category') or '',
            int(oi.get('w') or 0),
            int(oi.get('h') or 0),
        )
    return out


def read_iface():
    """tag -> {impl, props, grade, src}；容忍表格里的 **加粗** 与反引号"""
    out = {}
    if not os.path.exists(IFACE):
        return out
    with open(IFACE, 'r', encoding='utf-8') as f:
        for line in f:
            m = ROW_RE.match(line.strip())
            if not m:
                continue
            cells = [c.strip() for c in m.group('rest').split('|')]
            if len(cells) < 5:
                continue
            impl = cells[0].replace('*', '')
            props = cells[1].replace('*', '')
            # 表头：类名 | 实现方式 | 自有属性 | 只读 | 事件 | 档位 | 源文件
            grade = cells[4].replace('*', '').strip() if len(cells) > 4 else '—'
            src = cells[5].strip().strip('`') if len(cells) > 5 else '—'
            out[m.group('tag')] = dict(impl=impl, props=props, grade=grade, src=src)
    return out


def read_schema():
    """tag -> (总属性数, 自有属性数)"""
    if not os.path.exists(SCHEMA):
        return {}
    with open(SCHEMA, 'r', encoding='utf-8') as f:
        data = json.load(f)
    out = {}
    for ctl in data.get('controls', []):
        out[ctl['tag']] = len(ctl.get('properties', []))
    return out


def main():
    cat = read_catalog()
    iface = read_iface()
    schema = read_schema()

    missing_cn = [t for t in cat if t not in CN]
    if missing_cn:
        raise SystemExit('缺少中文名: %s' % ', '.join(missing_cn))

    lines = []
    lines.append('# MFCXaml 控件类名 / 中文名对照表')
    lines.append('')
    lines.append('> 由 `tools\\gen_control_names.py` 生成，勿手改。')
    lines.append('> XAML 标签（类名）就是 `guiload` / `xamlload` 里写的标签；中文名只用于界面与文档。')
    lines.append('> 档位口径见 `docs\\mfcxaml-控件接口清单.md`：S 完整 / A 基本 / B 薄 / C 仅布局。')
    lines.append('')

    total = 0
    for catkey in CAT_ORDER:
        rows = [(t, v) for t, v in cat.items() if v[0] == catkey]
        if not rows:
            continue
        rows.sort(key=lambda kv: kv[0].lower())
        lines.append('## %s（%s）' % (CAT_CN.get(catkey, catkey), len(rows)))
        lines.append('')
        lines.append('| 类名（XAML 标签） | 中文名 | 实现方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |')
        lines.append('|---|---|---|---|---|---|---|')
        for tag, (c, w, h) in rows:
            info = iface.get(tag, {})
            grade = info.get('grade') or '—'
            impl = info.get('impl') or '—'
            src = info.get('src') or '—'
            props = schema.get(tag)
            props_s = str(props) if props is not None else '—'
            lines.append('| `%s` | %s | %s | %s | %dx%d | %s | %s |'
                         % (tag, CN[tag], impl, grade, w, h, props_s, src))
            total += 1
        lines.append('')

    lines.append('## 合计')
    lines.append('')
    lines.append('共 **%d** 个标签：51 个主控件 + 5 个绘制图元（Line / Rectangle / Ellipse / Arc / Border）。'
                 % total)
    lines.append('')
    lines.append('不在本表的旧别名（已淘汰，勿再使用）：`TrackBar`（Slider 的旧名）、`Label`（TextBlock）、'
                 '`Edit`（TextBox）、`Progress`（ProgressBar）、`TreeView`（TreeCtrl）、`ListView`（ListCtrl）。')
    lines.append('')

    with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))
    print('写出 %s，共 %d 行控件' % (OUT, total))


if __name__ == '__main__':
    main()
