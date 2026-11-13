# -*- coding: utf-8 -*-
"""生成《MFCXaml 控件接口清单》——数据来自运行中的 PyMfcXaml.prop_schema() 与控件源码。

用法（构建后执行，schema 变了重跑）：
    & 'D:\\Program Files\\Python314\\python.exe' G:\\PyArx\\tools\\gen_control_doc.py
"""
import io
import json
import os
import sys

SCHEMA_JSON = r'G:\PyArx\docs\_schema.json'
OUT = r'G:\PyArx\docs\mfcxaml-控件接口清单.md'
BIN = r'G:\PyArx\X64\Release'

COMMON = ['Shape', 'BackColor', 'FrontColor', 'Foreground', 'BorderColor', 'BorderStyle',
          'BorderThickness', 'Radius', 'Opacity', 'TextAlign', 'VerticalAlignment',
          'TextWrapping', 'FontWeight', 'FontStyle', 'FontSize', 'FontFamily', 'Level',
          'TabIndex', 'Theme']

# 主标签 -> (实现方式, 源文件, 事件键)
INFO = {
    'Button':         ('原生 MFC Button', 'XamlButton.cpp', 'pointer'),
    'Label':      ('原生 MFC Static', 'XamlTextBlock.cpp', 'pointer'),
    'TextField':        ('原生 MFC Edit', 'XamlTextBox.cpp', 'text'),
    'ListBox':        ('原生 MFC ListBox', 'XamlListBox.cpp', 'select'),
    'CodeEditor':     ('C++ 自绘编辑器', 'XamlCodeEditor.cpp', 'text'),
    'CheckBox':       ('原生 MFC Button(BS_AUTOCHECKBOX)', 'XamlCheckBox.cpp', 'check'),
    'RadioButton':    ('原生 MFC Button(BS_AUTORADIO)', 'XamlRadioButton.cpp', 'radio'),
    'GroupBox':       ('原生 MFC Button(BS_GROUPBOX)', 'XamlGroupBox.cpp', 'pointer'),
    'ComboBox':       ('原生 MFC ComboBox', 'XamlComboBox.cpp', 'combo'),
    'ProgressBar':    ('原生 MFC msctls_progress32', 'XamlProgressBar.cpp', 'value'),
    'Slider':         ('原生 MFC msctls_trackbar32', 'XamlSlider.cpp', 'value'),
    'TabControl':     ('原生 MFC SysTabControl32', 'XamlTabControl.cpp', 'fairly'),
    'TreeView':       ('原生 MFC SysTreeView32', 'XamlTreeCtrl.cpp', 'select'),
    'ListCtrl':       ('原生 MFC SysListView32', 'XamlListCtrl.cpp', 'select'),
    'Choice':         ('原生 MFC ComboBox(CBS_DROPDOWNLIST)', 'XamlChoice.cpp', 'choice'),
    'DatePicker':     ('原生 MFC SysDateTimePick32', 'XamlDatePicker.cpp', 'date'),
    'Panel':          ('原生 MFC Static 容器', 'XamlPanel.cpp', 'panel'),
    'SplitView':       ('原生 MFC 自注册拖动条', 'XamlSplitter.cpp', 'splitter'),
    'StaticLine':     ('原生 MFC Static(SS_ETCHEDHORZ)', 'XamlStaticLine.cpp', 'pointer'),
    'PowerCanvas':    ('宿主自绘画布（配电规划图）', 'XamlPowerCanvas.cpp', 'canvas'),
    'Hyperlink':      ('原生 MFC SysLink', 'XamlHyperlink.cpp', 'link'),
    'TextArea':       ('原生 MFC RICHEDIT50W', 'XamlRichText.cpp', 'text'),
    'ToggleButton':   ('原生 MFC Button(BS_AUTOCHECKBOX 自绘)', 'XamlToggleButton.cpp', 'check'),
    'BitmapButton':   ('原生 MFC Button(BS_BITMAP)', 'XamlBitmapButton.cpp', 'pointer'),
    'Image':   ('原生 MFC Static(SS_BITMAP)', 'XamlStaticBitmap.cpp', 'pointer'),
    'GridCtrl':       ('原生 MFC 自注册网格', 'XamlGrid.cpp', 'select'),
    'ScrollView': ('原生 MFC 自注册滚动容器', 'XamlScrolledWindow.cpp', 'scrolled'),
    'CalendarCtrl':   ('原生 MFC SysMonthCal32', 'XamlCalendarCtrl.cpp', 'date'),
    'Switch':         ('自绘 skin #1 Switch', 'XamlSwitch.cpp', 'switch'),
    'SpinBox':        ('自绘 skin #2 Spin', 'XamlSpinBox.cpp', 'value'),
    'RangeSlider':    ('自绘 skin #3 Range', 'XamlRangeSlider.cpp', 'value'),
    'BusyIndicator':  ('自绘 skin #4 Busy', 'XamlBusyIndicator.cpp', 'busy'),
    'PageIndicator':  ('自绘 skin #5 Pages', 'XamlPageIndicator.cpp', 'fairly'),
    'Dial':           ('自绘 skin #6 Dial', 'XamlDial.cpp', 'value'),
    'Tumbler':        ('自绘 skin #7 Tumbler', 'XamlTumbler.cpp', 'value'),
    'ScrollBar':      ('自绘 skin #8 ScrollBar', 'XamlScrollBar.cpp', 'scrollbar'),
    'TableView':      ('自绘 skin #9 Table（独立控件套装）', 'XamlTableView.cpp', 'select'),
    'TreeTable':      ('自绘 skin #10 Tree', 'XamlTreeView.cpp', 'select'),
    'ToolBar':      ('自绘 skin #11 ToolBar', 'XamlToolBar.cpp', 'toolstrip'),
    'ToolTip':        ('自绘 skin #12 ToolTip', 'XamlToolTip.cpp', 'tooltip'),
    'Menu':           ('自绘 skin #13 Menu', 'XamlMenu.cpp', 'menu'),
    'MenuBar':      ('自绘 skin #14 MenuBar', 'XamlMenuBar.cpp', 'menustrip'),
    'Popup':          ('自绘 skin #15 Popup', 'XamlPopup.cpp', 'popup'),
    'VirtualList':    ('自绘 skin #16 ListView（虚拟化）', 'XamlListView.cpp', 'select'),
    'Page':           ('自绘 skin #17 Pager（XamlPageView）', 'XamlPage.cpp', 'fairly'),
    'GridView':       ('自绘 skin #18 GridView', 'XamlGridView.cpp', 'select'),
    'PathView':       ('自绘 skin #19 PathView', 'XamlPathView.cpp', 'select'),
    'QCanvas':        ('自绘 skin #20 QCanvas（命令脚本画布）', 'XamlQCanvas.cpp', 'canvas'),
    'Flickable':      ('自绘 skin #21 Flick', 'XamlFlickable.cpp', 'flick'),
    'StackView':      ('自绘 skin #17 Pager（XamlPageView 子类）', 'XamlStackView.h', 'fairly'),
    'SwipeView':      ('自绘 skin #17 Pager（XamlPageView 子类）', 'XamlSwipeView.h', 'fairly'),
}

EVENTS = {
    'pointer': 'click / dblclick / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus',
    'text': 'change / keydown / keyup / gotfocus / lostfocus / click / dblclick',
    'select': 'select / click / dblclick / scroll / gotfocus / lostfocus',
    'value': 'value / scroll / gotfocus / lostfocus / click',
    'check': 'click / checked / dblclick / gotfocus / lostfocus',
    'radio': 'click / checked / gotfocus / lostfocus',
    'fairly': 'click / select / gotfocus / lostfocus',
    'canvas': 'click / dblclick / mousedown / mouseup / mousemove / scroll / keydown / keyup / gotfocus / lostfocus',
    'combo': 'select / change / dropdown / dblclick / gotfocus / lostfocus',
    'choice': 'select / dropdown / dblclick / gotfocus / lostfocus',
    'scrollbar': 'value / scroll / gotfocus / lostfocus',
    'switch': 'click / checked / value / gotfocus / lostfocus',
    'panel': 'click / dblclick / mousedown / mouseup / mousemove',
    'splitter': 'mousedown / mouseup / mousemove / dblclick',
    'scrolled': 'scroll / click / gotfocus / lostfocus',
    'flick': 'scroll / click / mousedown / mouseup / mousemove',
    'link': 'click / dblclick / gotfocus / lostfocus',
    'menu': 'click / select',
    'menustrip': 'click / select',
    'toolstrip': 'click / select / dropdown',
    'tooltip': 'click',
    'popup': 'click / gotfocus / lostfocus',
    'date': 'select / value / dblclick / gotfocus / lostfocus',
    'busy': 'click / gotfocus / lostfocus',
}

ALIAS = {
    'Button': 'button, BTN',
    'Label': 'textblock, Label, label',
    'TextField': 'textbox, Edit, edit',
    'ListBox': 'listbox',
    'CodeEditor': 'codeeditor, CodeView, codeview, XmlEditor',
    'CheckBox': 'checkbox, Checkbox',
    'RadioButton': 'radiobutton, Radio, radio',
    'GroupBox': 'groupbox, Group, group',
    'ComboBox': 'combobox, Combo, combo',
    'ProgressBar': 'progressbar, Progress, progress',
    'Slider': 'slider',
    'TabControl': 'tabcontrol, TabWidget, tabwidget',
    'TreeView': 'treectrl, TreeView, tree',
    'ListCtrl': 'listctrl, ListView, listctrl_view',
    'Choice': 'choice',
    'DatePicker': 'datepicker, DatePickerCtrl',
    'Panel': 'panel',
    'SplitView': 'splitter, SplitterWindow',
    'StaticLine': 'staticline',
    'PowerCanvas': 'powercanvas, PlanCanvas',
    'Hyperlink': 'hyperlink, HyperlinkCtrl',
    'TextArea': 'richtext, RichTextCtrl',
    'ToggleButton': 'togglebutton, ToggleBtn',
    'BitmapButton': 'bitmapbutton, BitmapBtn',
    'Image': 'staticbitmap',
    'GridCtrl': 'gridctrl, DataGrid',
    'ScrollView': 'scrolledwindow',
    'CalendarCtrl': 'calendar, Calendar',
    'Switch': 'switch, ToggleSwitch',
    'SpinBox': 'spinbox, NumberBox, NumericUpDown',
    'RangeSlider': 'rangeslider, SliderRange',
    'BusyIndicator': 'busyindicator, Spinner, ActivityIndicator',
    'PageIndicator': 'pageindicator, PagerDots',
    'Dial': 'dial, Knob, Gauge',
    'Tumbler': 'tumbler, Wheel',
    'ScrollBar': 'scrollbar, VScrollBar, HScrollBar',
    'TableView': 'tableview, DataTable, GridTable',
    'TreeTable': 'treetable, HierarchicalTable',
    'ToolBar': 'toolstrip, ToolButtonBar',
    'ToolTip': 'tooltip, Hint',
    'Menu': 'menu, ContextMenu, PopupMenu',
    'MenuBar': 'menustrip, MenuBarControl, TopMenuBar',
    'Popup': 'popup, Overlay（Dialog / drawer 同实现）',
    'VirtualList': 'virtuallist, ListViewV, ModelList',
    'Page': 'PagedView, WizardPage',
    'StackView': 'stackview, PageView, CarouselView',
    'SwipeView': 'swipeview, PageSwipe',
    'GridView': 'gridview, TileView',
    'PathView': 'pathview, CoverFlow',
    'QCanvas': 'qcanvas, DrawCanvas, PaintCanvas, CanvasSurface, Painter',
    'Flickable': 'flickable, FlickArea',
    'StackView': 'stackview, PageView, CarouselView',
}

GRADE = [(5, 'S 完整'), (3, 'A 基本'), (1, 'B 薄'), (0, 'C 仅布局')]


def bare(tag):
    if len(tag) > 1 and tag[0] in 'Xx' and tag[1].isupper():
        return tag[1:]
    return tag


def grade(n):
    for lo, name in GRADE:
        if n >= lo:
            return name
    return 'C 仅布局'


def load_schema():
    """直接问运行中的扩展要 schema，避免读到过期的缓存 JSON。"""
    try:
        sys.path.insert(0, BIN)
        os.add_dll_directory(BIN)
        import PyMfcXaml as M
        raw = M.prop_schema()
        with io.open(SCHEMA_JSON, 'w', encoding='utf-8', newline='\n') as f:
            f.write(raw)
        return json.loads(raw)
    except Exception:
        if not os.path.isfile(SCHEMA_JSON):
            raise
        sys.stderr.write('WARN: PyMfcXaml 导入失败，回退到缓存 %s（可能是旧数据，请先构建）\n' % SCHEMA_JSON)
        with io.open(SCHEMA_JSON, encoding='utf-8') as f:
            return json.load(f)


def main():
    data = load_schema()
    controls = data['controls']
    rows = []
    own_total = 0
    common_total = 0
    stat = {}
    for c in controls:
        tag = c['tag']
        props = [p['name'] for p in c['properties']]
        own = [p['name'] for p in c['properties'] if p['name'] not in COMMON]
        ro = [p['name'] for p in c['properties'] if p.get('readOnly')]
        own_total += len(own)
        common_total += len(props) - len(own)
        impl, src, ev = INFO[bare(tag)]
        g = grade(len(own))
        stat[g] = stat.get(g, 0) + 1
        rows.append((tag, impl, src, own, ro, EVENTS[ev], g))

    buf = io.StringIO()
    w = buf.write
    w('# MFCXaml 控件接口清单\n\n')
    w('> 数据来源：运行中的 `PyMfcXaml.prop_schema()`（`XamlPropertySchemaJson()` 导出）+ '
      '`MfcXamlCore\\controls\\RegisterPyMfcControls.cpp`（标签注册）+ `XamlEvents.h`（事件契约）。\n')
    w('> 属性内容就是控件真正读写的字段，不是文档承诺。schema 变了重跑本文档：'
      '`python tools\\gen_control_doc.py`（需先构建出 `X64\\Release\\PyMfcXaml.pyd`）。\n\n')

    w('## 一、读数口径\n\n')
    w('- **通用属性 19 条**：每个控件都继承自 `XamlMfcControlBase`，是布局/外观字段 '
      '（`Shape / BackColor / FrontColor / Foreground / BorderColor / BorderStyle / BorderThickness / '
      'Radius / Opacity / TextAlign / VerticalAlignment / TextWrapping / FontWeight / FontStyle / '
      'FontSize / FontFamily / Level / TabIndex / Theme`）。它们不构成"封装程度"。\n')
    w('- **自有属性**：该控件自己声明、真正对应业务语义的字段（如 `Text`、`Items`、`Value`、`Columns`），'
      '本文档的"档位"只看这一列。\n')
    w('- **档位**：`S 完整` 自有属性 ≥5 · `A 基本` 3–4 · `B 薄` 1–2 · `C 仅布局` 0。\n')
    w('- **只读属性**：`Set == nullptr`，只能 `get`（如 `RowCount`、`ItemCount`、`PageCount`）。\n\n')

    w('## 二、总览\n\n')
    w('| 项 | 数量 |\n|---|---|\n')
    w('| 控件（注册主标签） | %d |\n' % len(controls))
    w('| 属性条目合计（含通用） | %d |\n' % sum(len(c['properties']) for c in controls))
    w('| 其中通用属性 | %d（%d 控件 × 19） |\n' % (common_total, len(controls)))
    w('| 其中自有属性 | %d |\n' % own_total)
    w('| 只读属性 | %d |\n' % sum(1 for c in controls for p in c['properties'] if p.get('readOnly')))
    w('| 事件键（内部名） | 15（click / dblclick / change / select / value / checked / dropdown / '
      'scroll / mousedown / mouseup / mousemove / keydown / keyup / gotfocus / lostfocus） |\n\n')
    w('档位分布：')
    for name in ('S 完整', 'A 基本', 'B 薄', 'C 仅布局'):
        w('%s **%d** 个 · ' % (name, stat.get(name, 0)))
    w('\n\n')

    w('## 三、逐控件清单\n\n')
    w('| 控件 | 实现 | 自有属性（读写） | 只读 | 事件 | 档位 | 源文件 |\n')
    w('|---|---|---|---|---|---|---|\n')
    for tag, impl, src, own, ro, ev, g in rows:
        w('| **%s** | %s | %s | %s | %s | %s | `%s` |\n' % (
            tag, impl, ' / '.join(own) if own else '—',
            ' / '.join(ro) if ro else '—', ev, g, src))
    w('\n')

    w('## 四、分档结论\n\n')
    w('**S 完整（自有 ≥5，可直接当业务控件用）**：')
    w('、'.join('%s(%d)' % (t, len(o)) for t, i, s, o, r, e, g in rows if g == 'S 完整'))
    w('\n\n**A 基本（3–4，够用但还缺边角能力）**：')
    w('、'.join('%s(%d)' % (t, len(o)) for t, i, s, o, r, e, g in rows if g == 'A 基本'))
    w('\n\n**B 薄（1–2，只有主语义那一个字段）**：')
    w('、'.join('%s(%d)' % (t, len(o)) for t, i, s, o, r, e, g in rows if g == 'B 薄'))
    w('\n\n**C 仅布局（自有 0，属性面上等同一块带样式的面板）**：')
    w('、'.join(t for t, i, s, o, r, e, g in rows if g == 'C 仅布局'))
    w('\n\n')
    w('C 档不是"没实现"——这些控件在 C++ 里都有真实原生窗口/自绘实现、事件也照常派发，'
      '只是**业务字段没有暴露成属性**。要让它们进设计器可配置，得按需要补属性表条目；'
      '这也是 `plugins` 层用 XAML 组合补位的典型对象（组合态用子控件属性，不硬造新属性）。\n\n')

    w('## 五、命名规范\n\n')
    w('标签统一为 `X + 本意名`（`XButton` / `XTextField` / `XStackPanel` / `XWindow`），'\
      '只保留唯一一个标签，历史别名（`BTN` / `Edit` / `TreeView` / `Progress` 等）已全部删除。\n'\
      '解析入口统一去前缀取本意名，`<Button>` 这类旧写法不再注册。\n\n')

    w('## 六、本轮已修 / 剩余缺口\n\n')
    w('**本轮已修（v0.10.17）**\n\n')
    w('1. `TextArea` 属性重复：`FontSize`、`Foreground` 不再由控件自声明，统一走基类外观字段，'
      '由 `onAppearanceChanged` 转成 `CHARFORMAT`（`XamlRichText.cpp`），schema 已无重名条目。\n')
    w('2. `CalendarCtrl` 补上 `Value`（`YYYY-MM-DD`，读写）：宿主新增 '
      '`getCalendarDate` / `setCalendarDate`（`MCM_GETCURSEL` / `MCM_SETCURSEL`）。\n')
    w('3. `Flickable` 补上 `OffsetX` / `OffsetY`（读写，落到宿主 `setFlickOffset`），'
      '不再只能靠手势改视口。\n')
    w('4. 事件契约：`combobox` / `choice` 补 `keydown` / `keyup`（宿主本来就把 `WM_KEYDOWN` '
      '统一派发成 `keydown`，缺的只是契约条目）。\n\n')
    w('**剩余缺口（按实测，不含猜测）**\n\n')
    w('1. **只读属性共 7 个**：`TableView.RowCount`、`Menu.SelectedIndex`、`VirtualList.ItemCount`、'
      '`GridView.ItemCount`、`Page.PageCount`、`StackView.PageCount`、`SwipeView.PageCount`。'
      '只有 `Menu.SelectedIndex` 是设计上的只读回读槽（`XamlMenu.cpp` 里 `Set` 为空，只反射当前选中）；'
      '若需要程序化选中菜单项，得补 `setMenuSelected` 宿主接口 + 属性 `Set`。其余 6 个是计数类，只读合理。\n')
    w('2. **`Page` / `StackView` / `SwipeView` 是同一实现**（`XamlPageView`，skin #17），'
      '三者属性都是 `SelectedIndex` + `PageCount`，差别仅在语义命名；后续若要真分页/滑动行为，'
      '需要在 `XamlPageView` 上分模式，而不是继续加别名。\n')
    w('3. **仍有 5 个控件自有属性为 0**：`Panel`、`SplitView`、`StaticLine`、`BusyIndicator`、'
      '`Flickable`（Flickable 已补偏移，其余 4 个仍只有布局/外观）；`BusyIndicator` 要补 `Running` / '
      '`Interval` 得先给 skinbox 加状态接口。\n')
    w('4. **B 档 30 个控件多数只有一个主语义字段**，缺的是"第二层"能力（如 `Button.Icon` / '
      '`ListBox.MultiSelect` 之外的 `ItemHeight`、`TreeCtrl.Indent` / `NodeState`），'
      '要按实际使用场景逐个补，不预造属性。\n')

    with io.open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write(buf.getvalue())
    print('written', OUT, len(buf.getvalue()))


main()
