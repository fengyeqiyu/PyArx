# -*- coding: utf-8 -*-
"""PyMfcXaml 全控件可视演示：每个控件单独一个窗口，全部保持打开、不关闭。

用法：python.exe G:\PyArx\tests\mfc_controls\demo_all_controls.py
- 26 个控件各自一个窗口，按网格排布，标题为「序号 + 控件名」
- 每个窗口建好后写入示例内容，并核对原生类名/尺寸
- 之后无限泵消息，窗口由使用者自己关闭（全部关闭后脚本才退出）
"""
import ctypes
import os
import sys
import tempfile
from ctypes import wintypes

import PyMfcXaml as p

user32 = ctypes.windll.user32
HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, 'demo_all_controls.log')

WM_CLOSE = 0x0010
SWP_NOSIZE = 0x0001
SWP_NOZORDER = 0x0004

# tag, 标题, 宽, 高, 控件高度, XAML 额外属性, 可选内嵌子控件
CONTROLS = [
    ('Button', '按钮', 300, 200, 46, 'Content="确定"', None),
    ('Label', '文本块', 300, 200, 60, 'Content="一段说明文字"', None),
    ('TextField', '文本框', 300, 200, 34, 'Text="初始文本"', None),
    ('CheckBox', '复选框', 300, 200, 30, 'Content="同意条款"', None),
    ('RadioButton', '单选按钮', 300, 200, 30, 'Content="选项 A"', None),
    ('GroupBox', '分组框', 300, 200, 90, 'Content="分组框"', None),
    ('ComboBox', '下拉框', 300, 210, 34, None, None),
    ('ListBox', '列表框', 300, 220, 120, None, None),
    ('ProgressBar', '进度条', 300, 200, 26, None, None),
    ('Slider', '滑块', 300, 200, 40, None, None),
    ('TabControl', '选项卡', 300, 220, 120, None, None),
    ('TreeView', '树形控件', 300, 220, 120, None, None),
    ('ListCtrl', '列表控件', 300, 220, 110, None, None),
    ('Choice', '选择框', 300, 200, 34, None, None),
    ('DatePicker', '日期选择', 300, 200, 34, None, None),
    ('Panel', '面板容器', 300, 200, 90, None, 'Label'),
    ('SplitView', '分隔条', 300, 200, 20, None, None),
    ('StaticLine', '分隔线', 300, 200, 12, None, None),
    ('Hyperlink', '超链接', 300, 200, 34, 'Content="打开示例链接"', None),
    ('TextArea', '富文本', 300, 220, 110, None, None),
    ('ToggleButton', '切换按钮', 300, 200, 46, 'Content="切换模式"', None),
    ('BitmapButton', '位图按钮', 300, 200, 46, 'Content="运行"', None),
    ('Image', '静态位图', 300, 200, 70, None, None),
    ('GridCtrl', '表格控件', 300, 220, 110, None, None),
    ('ScrollView', '滚动框', 300, 220, 90, None, 'Label'),
    ('CalendarCtrl', '日历', 320, 240, 170, None, None),
    # QML 对齐（第三批）
    ('Switch', '开关', 300, 200, 30, None, None),
    ('SpinBox', '数字输入', 300, 200, 32, None, None),
    ('RangeSlider', '范围滑块', 300, 200, 34, None, None),
    ('BusyIndicator', '忙碌指示', 300, 200, 56, None, None),
    ('PageIndicator', '页码点', 300, 200, 22, None, None),
    ('Dial', '圆弧旋钮', 300, 200, 64, None, None),
    ('Tumbler', '滚动选择', 300, 200, 72, None, None),
    ('ScrollBar', '滚动条', 300, 200, 22, None, None),
    # QML 对齐（第四批）：表格类 + 工具栏
    ('TableView', '数据表格', 300, 200, 96, None, None),
    ('TreeTable', '树形表格', 300, 200, 96, None, None),
    ('ToolBar', '工具栏', 300, 200, 34, None, None),
]

EXPECT_CLASS = {
    'Button': 'Button', 'CheckBox': 'Button', 'RadioButton': 'Button',
    'GroupBox': 'Button', 'ToggleButton': 'Button', 'BitmapButton': 'Button',
    'Label': 'Static', 'TextField': 'Edit', 'ListBox': 'ListBox',
    'ComboBox': 'ComboBox', 'ProgressBar': 'msctls_progress32', 'Slider': 'msctls_trackbar32',
    'TabControl': 'SysTabControl32', 'TreeView': 'SysTreeView32', 'ListCtrl': 'SysListView32',
    'DatePicker': 'SysDateTimePick32', 'CalendarCtrl': 'SysMonthCal32',
    'Hyperlink': 'SysLink', 'TextArea': 'RICHEDIT50W',
    'Switch': 'PyMfcSkinBox', 'SpinBox': 'PyMfcSkinBox', 'RangeSlider': 'PyMfcSkinBox',
    'BusyIndicator': 'PyMfcSkinBox', 'PageIndicator': 'PyMfcSkinBox',
    'Dial': 'PyMfcSkinBox', 'Tumbler': 'PyMfcSkinBox', 'ScrollBar': 'PyMfcSkinBox',
    'TableView': 'PyMfcSkinBox', 'TreeTable': 'PyMfcSkinBox', 'ToolBar': 'PyMfcSkinBox',
}

EnumChildProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

_lines = []


def out(text):
    print(text, flush=True)
    _lines.append(text)


def find_child_class(hwnd, cid):
    found = {}

    def cb(child, _):
        if user32.GetDlgCtrlID(child) == cid:
            buf = ctypes.create_unicode_buffer(128)
            user32.GetClassNameW(child, buf, 128)
            rect = wintypes.RECT()
            user32.GetWindowRect(child, ctypes.byref(rect))
            found.setdefault(cid, (child, buf.value, rect))
        return True

    user32.EnumChildWindows(hwnd, EnumChildProc(cb), 0)
    return found.get(cid)


def decorate(w, tag, cid):
    """给各控件填上示例内容，让窗口里看得出状态。"""
    try:
        if tag in ('ComboBox', 'Choice', 'ListBox'):
            w.clearStrings(cid)
            for item in ('甲选项', '乙选项', '丙选项'):
                w.addString(cid, item)
            w.setCurSel(cid, 1)
        elif tag == 'ProgressBar':
            w.setRange(cid, 0, 100)
            w.setPos(cid, 65)
        elif tag == 'Slider':
            w.setRange(cid, 0, 100)
            w.setPos(cid, 45)
        elif tag in ('CheckBox', 'ToggleButton', 'Switch'):
            if tag == 'Switch':
                w.setSwitch(cid, True)
            else:
                w.setCheck(cid, True)
        elif tag == 'SpinBox':
            w.setSpinRange(cid, 0, 99)
            w.setSpin(cid, 30)
        elif tag == 'RangeSlider':
            w.setRange(cid, 0, 100)
            w.setRangeValues(cid, 25, 75)
        elif tag == 'PageIndicator':
            w.setPageCount(cid, 5)
            w.setPageIndex(cid, 2)
        elif tag == 'Dial':
            w.setRange(cid, 0, 100)
            w.setDial(cid, 40)
        elif tag == 'Tumbler':
            w.setTumblerCount(cid, 12)
            w.setTumblerValue(cid, 3)
        elif tag == 'ScrollBar':
            w.setScrollBarRange(cid, 0, 100, 20, False)
            w.setScrollBarValue(cid, 35)
        elif tag == 'TableView':
            w.setTableColumns(cid, '名称\t数值\t状态')
            for row in ('甲\t30\t正常', '乙\t12\t警告', '丙\t21\t正常', '丁\t48\t正常'):
                w.addTableRow(cid, row)
            w.setTableSelectedRow(cid, 1)
        elif tag == 'TreeTable':
            w.addTreeItem(cid, '项目根', 0, True)
            w.addTreeItem(cid, '图纸 A', 1, False)
            w.addTreeItem(cid, '图纸 B', 1, False)
            w.addTreeItem(cid, '图层组', 1, True)
            w.addTreeItem(cid, '墙体', 2, False)
            w.setTreeSelectedRow(cid, 1)
        elif tag == 'ToolBar':
            for it in ('新建', '打开', '保存', '运行'):
                w.addToolBarItem(cid, it)
            w.setToolBarSelected(cid, 2)
        elif tag in ('TextField', 'TextArea'):
            w.setText(cid, '可以在里面输入文字')
        elif tag == 'TabControl':
            # 上一版这里是 pass，页签里什么都没有，看上去就是一块白板
            w.addTabItem(cid, '常规')
            w.addTabItem(cid, '高级')
            w.addTabItem(cid, '关于')
        elif tag == 'TreeView':
            # addString 对 TreeView 无效，必须走 addTreeNode
            w.addTreeNode(cid, '根节点', -1)
            w.addTreeNode(cid, '子节点 A', 0)
            w.addTreeNode(cid, '子节点 B', 0)
        elif tag in ('ListCtrl', 'GridCtrl'):
            # addString 对 ListView 无效，必须 addColumn + addRow（\t 分列）
            w.addColumn(cid, '名称', 100)
            w.addColumn(cid, '数值', 80)
            w.addRow(cid, '甲\t12')
            w.addRow(cid, '乙\t34')
            w.addRow(cid, '丙\t56')
        elif tag == 'Image':
            # 不装图就是一块空白，看上去和坏掉没区别
            w.setImage(cid, os.path.join(HERE, 'res', 'dot.png'))
        elif tag in ('Button', 'Label', 'BitmapButton', 'Hyperlink', 'RadioButton'):
            pass
    except Exception as exc:                      # 装饰失败不该阻断演示
        out(f'    装饰失败（{tag}）：{exc}')


def main():
    tmpdir = tempfile.mkdtemp(prefix='pymfc_demo_')
    windows = []
    ok_count = 0
    fail_count = 0

    for index, (tag, label, ww, hh, ch, attrs, inner) in enumerate(CONTROLS, start=1):
        title = f'{index:02d} {tag} - {label}'
        # 生成该控件的单窗口 XAML
        inner_xml = ''
        if inner:
            inner_xml = f'      <{inner} x:Name="inner1" Content="容器内文本" Height="24" />\n'
        if inner:
            body = (f'    <{tag} x:Name="ctrl" Height="{ch}">\n{inner_xml}    </{tag}>\n')
        else:
            attr_text = (attrs + ' ') if attrs else ''
            body = f'    <{tag} x:Name="ctrl" {attr_text}Height="{ch}" />\n'
        xaml = (
            '<?xml version="1.0" encoding="utf-8"?>\n'
            '<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"\n'
            f'        Title="{title}" Width="{ww}" Height="{hh}">\n'
            '  <XStackPanel Margin="12" Spacing="6">\n'
            f'{body}'
            '  </XStackPanel>\n'
            '</XWindow>\n'
        )
        path = os.path.join(tmpdir, f'{index:02d}_{tag}.xaml')
        with open(path, 'w', encoding='utf-8') as fh:
            fh.write(xaml)

        names = {}
        try:
            win = p.guiload(path, names=names)
        except Exception as exc:
            out(f'FAIL  {title}  加载失败：{exc}')
            fail_count += 1
            continue

        cid = names.get('ctrl')
        decorate(win, tag, cid)

        win.show()
        p.pump(80)                                 # 让它先画出首帧

        # 网格排布：6 列，窗口互不遮挡
        col = (index - 1) % 6
        row = (index - 1) // 6
        x = 30 + col * (ww + 12)
        y = 30 + row * 236
        user32.SetWindowPos(win.hwnd, None, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER)
        p.pump(30)

        child = find_child_class(win.hwnd, cid) if cid is not None else None
        if child is None:
            out(f'FAIL  {title}  id={cid} 找不到控件窗口')
            fail_count += 1
        else:
            _hwnd, cls, rect = child
            expect = EXPECT_CLASS.get(tag)
            size = (rect.right - rect.left, rect.bottom - rect.top)
            class_ok = (expect is None) or (cls == expect)
            if class_ok and size[0] > 0 and size[1] > 0:
                ok_count += 1
                out(f'ok    {title}  id={cid} class={cls} size={size[0]}x{size[1]}  '
                    f'win={hex(win.hwnd)}')
            else:
                fail_count += 1
                out(f'FAIL  {title}  id={cid} class={cls}(期望 {expect}) size={size[0]}x{size[1]}')

        windows.append((title, win))

    out('')
    out(f'共建窗口 {len(windows)} 个：通过 {ok_count}，失败 {fail_count}。'
        f'窗口全部保持打开，请自行关闭（全部关闭后脚本退出）。')

    with open(LOG, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(_lines) + '\n')

    # 无限泵消息：不调用 close()，等使用者把窗口一个个关掉
    try:
        while True:
            alive = [w for _t, w in windows if user32.IsWindow(w.hwnd)]
            if not alive:
                out('全部窗口已关闭，退出。')
                break
            p.pump(100)
    except KeyboardInterrupt:
        out('收到 Ctrl+C，脚本退出（窗口随进程结束）。')


if __name__ == '__main__':
    sys.exit(main())
