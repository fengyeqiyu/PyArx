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

# tag -> (QML 对应类型, 对应方式)
# 对应方式：同名 = QML 里有同名同义控件（换个引擎基本能直接搬）
#           组合 = QML 用两三个控件拼出等价能力
#           自绘 = QML 侧要用 Canvas / Shape 画
#           无   = QML 无等价，需另找方案
QML = {
    'Button': ('Button', '同名'),
    'CheckBox': ('CheckBox', '同名'),
    'RadioButton': ('RadioButton', '同名'),
    'ToggleButton': ('Button + checkable', '组合'),
    'BitmapButton': ('Button + icon.source', '组合'),
    'TextBox': ('TextField / TextArea', '同名'),
    'ComboBox': ('ComboBox', '同名'),
    'ListBox': ('ListView', '组合'),
    'Choice': ('ComboBox（editable:false）', '同名'),
    'DatePicker': ('TextField + MonthGrid', '组合'),
    'CodeEditor': ('TextArea（等宽字体 + 行号槽）', '组合'),
    'Slider': ('Slider', '同名'),
    'Switch': ('Switch', '同名'),
    'SpinBox': ('SpinBox', '同名'),
    'RangeSlider': ('RangeSlider', '同名'),
    'Dial': ('Dial', '同名'),
    'Tumbler': ('Tumbler', '同名'),
    'ScrollBar': ('ScrollBar / ScrollView', '同名'),
    'TextBlock': ('Label / Text', '同名'),
    'ProgressBar': ('ProgressBar', '同名'),
    'Hyperlink': ('Label + MouseArea（link 样式）', '组合'),
    'RichText': ('TextArea（textFormat: RichText）', '组合'),
    'StaticBitmap': ('Image', '同名'),
    'CalendarCtrl': ('MonthGrid + DayOfWeekRow', '组合'),
    'BusyIndicator': ('BusyIndicator', '同名'),
    'PageIndicator': ('PageIndicator', '同名'),
    'TableView': ('TableView（model + delegate）', '组合'),
    'TreeTable': ('TreeView（6.x）+ 多列 delegate', '组合'),
    'VirtualList': ('ListView（cacheBuffer 虚拟化）', '组合'),
    'StackView': ('StackView', '同名'),
    'SwipeView': ('SwipeView', '同名'),
    'GridView': ('GridView', '同名'),
    'PathView': ('PathView', '同名'),
    'Page': ('Page', '同名'),
    'GroupBox': ('GroupBox', '同名'),
    'TabControl': ('TabBar + StackLayout', '组合'),
    'Border': ('Rectangle（border.*）', '组合'),
    'Panel': ('Item / Pane', '组合'),
    'Splitter': ('SplitView', '组合'),
    'TreeCtrl': ('TreeView', '组合'),
    'ListCtrl': ('ListView / TableView（单列）', '组合'),
    'StaticLine': ('Rectangle（height: 1）/ HorizontalDivider', '组合'),
    'GridCtrl': ('GridLayout / TableView', '组合'),
    'ScrolledWindow': ('ScrollView', '组合'),
    'Flickable': ('Flickable', '同名'),
    'ToolStrip': ('ToolBar', '组合'),
    'MenuStrip': ('MenuBar', '组合'),
    'ToolTip': ('ToolTip', '同名'),
    'Menu': ('Menu', '同名'),
    'Popup': ('Popup', '同名'),
    'PowerCanvas': ('Canvas / Shape', '自绘'),
    'QCanvas': ('Canvas / Item', '自绘'),
    'Line': ('Shape + Path', '自绘'),
    'Rectangle': ('Rectangle', '同名'),
    'Ellipse': ('Shape（PathArc）', '自绘'),
    'Arc': ('Shape + PathAngleArc', '自绘'),
}

# 属性映射：MFCXaml 通用属性 -> QML
PROP_MAP = [
    ('BackColor / FrontColor', 'background: Rectangle { color: ... }（QML 无全局底色属性）'),
    ('Foreground / FrontColor(文字)', 'color（Text / Label）或 palette.text'),
    ('BorderColor / BorderStyle / BorderThickness', 'Rectangle 的 border.color / border.width；Sunken、Raised 需自绘'),
    ('Radius', 'radius'),
    ('Opacity', 'opacity（0-255 折算成 0.0-1.0）'),
    ('TextAlign', 'horizontalAlignment / verticalAlignment'),
    ('TextWrapping', 'wrapMode'),
    ('FontSize / FontFamily / FontWeight / FontStyle', 'font.pixelSize / font.family / font.weight / font.italic'),
    ('Level', 'z（叠放顺序，语义等价）'),
    ('TabIndex', '无对应：Qt 用 activeFocusOnTab + KeyNavigation'),
    ('Theme', '无对应：Qt 用 qtquickcontrols2.conf / 自定义 Style'),
]

# 事件映射：XAML 回调键 -> QML 信号处理器
EVENT_MAP = [
    ('click', 'onClicked（Button）/ MouseArea.onClicked'),
    ('dblclick', 'MouseArea.onDoubleClicked'),
    ('change', 'onTextEdited / onTextChanged / onAccepted'),
    ('select', 'onCurrentIndexChanged / onCurrentItemChanged'),
    ('value', 'onValueChanged / onPositionChanged'),
    ('checked', 'onCheckedChanged / onToggled'),
    ('dropdown', 'onPressedChanged（Popup 开合）'),
    ('scroll', 'WheelHandler.onWheel'),
    ('mousedown / mouseup / mousemove', 'MouseArea.onPressed / onReleased / onPositionChanged'),
    ('keydown / keyup', 'Keys.onPressed / Keys.onReleased'),
    ('gotfocus / lostfocus', 'onActiveFocusChanged'),
]


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
    lines.append('# MFCXaml 控件类名 / 中文名 / QML 对照表')
    lines.append('')
    lines.append('> 由 `tools\\gen_control_names.py` 生成，勿手改。')
    lines.append('> XAML 标签（类名）就是 `guiload` / `xamlload` 里写的标签；中文名只用于界面与文档。')
    lines.append('> 档位口径见 `docs\\mfcxaml-控件接口清单.md`：S 完整 / A 基本 / B 薄 / C 仅布局。')
    lines.append('> QML 一列是"同一套界面换引擎时落在哪个类型"，同名即语义基本等价。')
    lines.append('')

    total = 0
    kind_count = {}
    for catkey in CAT_ORDER:
        rows = [(t, v) for t, v in cat.items() if v[0] == catkey]
        if not rows:
            continue
        rows.sort(key=lambda kv: kv[0].lower())
        lines.append('## %s（%s）' % (CAT_CN.get(catkey, catkey), len(rows)))
        lines.append('')
        lines.append('| 类名（XAML 标签） | 中文名 | QML 对应 | 对应方式 | 档位 | 默认尺寸 | 属性数 | 源文件 |')
        lines.append('|---|---|---|---|---|---|---|---|')
        for tag, (c, w, h) in rows:
            info = iface.get(tag, {})
            grade = info.get('grade') or '—'
            src = info.get('src') or '—'
            props = schema.get(tag)
            props_s = str(props) if props is not None else '—'
            qml, kind = QML[tag]
            kind_count[kind] = kind_count.get(kind, 0) + 1
            lines.append('| `%s` | %s | %s | %s | %s | %dx%d | %s | %s |'
                         % (tag, CN[tag], qml, kind, grade, w, h, props_s, src))
            total += 1
        lines.append('')

    lines.append('## 属性对应')
    lines.append('')
    lines.append('| MFCXaml 属性 | QML 写法 |')
    lines.append('|---|---|')
    for a, b in PROP_MAP:
        lines.append('| %s | %s |' % (a, b))
    lines.append('')

    lines.append('## 事件对应')
    lines.append('')
    lines.append('| XAML 回调键 | QML 信号处理器 |')
    lines.append('|---|---|')
    for a, b in EVENT_MAP:
        lines.append('| `%s` | `%s` |' % (a, b))
    lines.append('')

    lines.append('## 合计与对应情况')
    lines.append('')
    lines.append('共 **%d** 个标签：51 个主控件 + 5 个绘制图元（Line / Rectangle / Ellipse / Arc / Border）。'
                 % total)
    lines.append('')
    for k in ('同名', '组合', '自绘', '无'):
        if kind_count.get(k):
            lines.append('- **%s**：%d 个' % (k, kind_count[k]))
    lines.append('')
    lines.append('同名的那批（Button / CheckBox / ComboBox / Slider / Switch / SpinBox / Dial / Tumbler / '
                 'StackView / SwipeView / GridView / PathView / Page / Flickable / ToolTip / Menu / Popup 等）'
                 '换到 QML 只要改标签名和属性写法，布尔与数值语义一致，可以直接对搬。')
    lines.append('')
    lines.append('三处必须注意的结构差异：')
    lines.append('')
    lines.append('1. **表格与列表是 model + delegate**：MFCXaml 的 `Rows` / `Items` / `Columns` 是字符串直填，'
                 'QML 侧要换 `ListModel` 或 `QAbstractTableModel` + delegate，这是迁移时工作量最大的地方。')
    lines.append('2. **没有原生 TreeTable / GridCtrl**：QML 需 TreeView 或多列 delegate 自己组，'
                 '不是换个标签就能完成。')
    lines.append('3. **自绘控件（PowerCanvas / QCanvas / Line / Ellipse / Arc）**：MFCXaml 走 GDI 自绘，'
                 'QML 侧对应 Canvas 或 Shape，绘制代码要重写而不是映射。')
    lines.append('')
    lines.append('另外两条工程差异：QML 默认按 96 DPI 的像素单位布局，MFC 是物理像素，'
                 '尺寸要按 `devicePixelRatio` 折算；QML 的焦点与 Tab 顺序靠 `activeFocusOnTab` + `KeyNavigation`，'
                 '不像 XAML 有 `TabIndex`。')
    lines.append('')
    lines.append('不在本表的旧别名（已淘汰，勿再使用）：`TrackBar`（Slider 的旧名）、`Label`（TextBlock）、'
                 '`Edit`（TextBox）、`Progress`（ProgressBar）、`TreeView`（TreeCtrl）、`ListView`（ListCtrl）。')
    lines.append('')

    with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))
    print('写出 %s，共 %d 行控件；对应方式 %s' % (OUT, total, kind_count))


if __name__ == '__main__':
    main()
