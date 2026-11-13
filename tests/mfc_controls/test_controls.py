# -*- coding: utf-8 -*-
"""PyMfcXaml 控件全量测试：26 类控件 + 属性/交互/事件 + Panel 容器。

用法：python.exe G:\\PyArx\\tests\\mfc_controls\\test_controls.py
退出码 0 = 全通过；非 0 = 有失败项（明细见输出）。
"""
import ctypes
import os
import sys
from ctypes import wintypes

import PyMfcXaml as p

HERE = os.path.dirname(os.path.abspath(__file__))
user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32

# ---- Win32 常量 ----
WM_COMMAND = 0x0111
WM_HSCROLL = 0x0114
WM_CHAR = 0x0102
WM_CLOSE = 0x0010
BM_CLICK = 0x00F5
BM_SETCHECK = 0x00F1
BST_CHECKED = 1
BN_CLICKED = 0
EN_CHANGE = 0x0300
LBN_SELCHANGE = 1
TB_THUMBTRACK = 5
SW_SHOW, SW_HIDE = 5, 0

EnumChildProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

results = []      # (section, item, ok, detail)


def record(section, item, ok, detail=''):
    results.append((section, item, ok, detail))
    print(('ok    ' if ok else 'FAIL  ') + f'[{section}] {item}' + ('  ' + detail if detail else ''))


def info(section, item, detail):
    results.append((section, item, None, detail))
    print(f'info  [{section}] {item}  {detail}')


def enum_children(top):
    """递归枚举全部后代窗口：id -> (hwnd, class, rect)"""
    found = {}

    def walk(hwnd):
        def cb(child, _):
            cid = user32.GetDlgCtrlID(child)
            cls = ctypes.create_unicode_buffer(256)
            user32.GetClassNameW(child, cls, 256)
            rect = wintypes.RECT()
            user32.GetWindowRect(child, ctypes.byref(rect))
            found.setdefault(cid, (child, cls.value, rect))
            walk(child)
            return True
        user32.EnumChildWindows(hwnd, EnumChildProc(cb), 0)

    walk(top)
    return found


def ctrl_map(w, names):
    """name -> (id, hwnd, class, rect)"""
    kids = enum_children(w.hwnd)
    out = {}
    for name, cid in names.items():
        if cid in kids:
            out[name] = (cid, kids[cid][0], kids[cid][1], kids[cid][2])
        else:
            out[name] = (cid, None, None, None)
    return out


def parent_of(hwnd):
    return user32.GetParent(hwnd)


def section(title):
    print('\n== ' + title + ' ==')


# ---------------------------------------------------------------------------
EXPECT_CLASS = {
    'Button': 'Button', 'CheckBox': 'Button', 'RadioButton': 'Button',
    'GroupBox': 'Button', 'ToggleButton': 'Button', 'IconButton': 'Button',
    'Label': 'Static', 'TextField': 'Edit', 'ListBox': 'ListBox',
    'ComboBox': 'ComboBox', 'ProgressBar': 'msctls_progress32', 'Slider': 'msctls_trackbar32',
    'TabBar': 'SysTabControl32', 'TreeView': 'SysTreeView32', 'ListCtrl': 'SysListView32',
    'DatePicker': 'SysDateTimePick32', 'MonthGrid': 'SysMonthCal32',
}

TAG_OF = {
    'lblHead': 'Label', 'txtInput': 'TextField', 'btnOk': 'Button', 'chkAgree': 'CheckBox',
    'radA': 'RadioButton', 'radB': 'RadioButton', 'grpTest': 'GroupBox', 'cmbCity': 'ComboBox',
    'lstItems': 'ListBox', 'prgWork': 'ProgressBar', 'sldLevel': 'Slider',
    'lblStatus': 'Label', 'btnHidden': 'Button',
    'tabMain': 'TabBar', 'treeFiles': 'TreeView', 'lstView': 'ListCtrl', 'choPick': 'Choice',
    'dtpDate': 'DatePicker', 'pnlHost': 'Pane', 'splVert': 'SplitView', 'lnSep': 'Divider',
    'lnkOpen': 'Hyperlink', 'rtfNote': 'TextArea', 'tglMode': 'ToggleButton',
    'bbtnRun': 'IconButton', 'sbmpPreview': 'Image', 'grdData': 'GridLayout',
    'scrArea': 'ScrollView', 'calPick': 'MonthGrid',
}


def run_window(xaml_name, title, tag_of):
    section(title)
    path = os.path.join(HERE, xaml_name)
    names = {}
    w = p.guiload(path, names=names)
    w.show()
    p.pump(400)

    cmap = ctrl_map(w, names)
    record(title, 'XAML 加载', True, f'{len(names)} 个命名控件')
    record(title, '窗口句柄有效', bool(user32.IsWindow(w.hwnd)), hex(w.hwnd))
    record(title, '窗口可见', bool(user32.IsWindowVisible(w.hwnd)))
    record(title, '控件 ID 分配', all(v[1] is not None for v in cmap.values()),
           str({k: v[0] for k, v in cmap.items()}))

    # 原生类断言
    for name, (cid, hwnd, cls, rect) in cmap.items():
        tag = tag_of.get(name, '?')
        expect = EXPECT_CLASS.get(tag)
        if hwnd is None:
            record(title, f'{name} ({tag}) 存在', False, '找不到窗口')
            continue
        if expect is None:
            info(title, f'{name} ({tag})', f'class={cls} id={cid}')
        else:
            record(title, f'{name} ({tag}) 类名', cls == expect, f'实际 {cls} / 期望 {expect}')
        ww = rect.right - rect.left
        hh = rect.bottom - rect.top
        record(title, f'{name} 尺寸非零', ww > 0 and hh > 0, f'{ww}x{hh}')
    return w, cmap, names


def test_basic(w, cmap, names):
    section('基础控件行为')
    btn = names['btnOk']; txt = names['txtInput']; lbl = names['lblHead']; chk = names['chkAgree']
    rad = names['radA']; grp = names['grpTest']; cmb = names['cmbCity']; lst = names['lstItems']
    prg = names['prgWork']; sld = names['sldLevel']; hid = names['btnHidden']

    # 文本往返
    w.setText(txt, '测试文本 ABC')
    record('文本', 'TextBox setText/getText', w.getText(txt) == '测试文本 ABC', w.getText(txt))
    w.setReadOnly(txt, True)
    record('文本', 'TextBox setReadOnly/getReadOnly', w.getReadOnly(txt) is True, str(w.getReadOnly(txt)))
    w.setReadOnly(txt, False)

    w.setText(btn, '按钮新标题')
    record('文本', 'Button setText/getText', w.getText(btn) == '按钮新标题', w.getText(btn))
    w.setText(lbl, '标签新文本')
    record('文本', 'TextBlock setText/getText', w.getText(lbl) == '标签新文本', w.getText(lbl))

    # 勾选
    w.setCheck(chk, True)
    record('勾选', 'CheckBox setCheck(True)', w.getCheck(chk) == BST_CHECKED, str(w.getCheck(chk)))
    w.setCheck(chk, False)
    record('勾选', 'CheckBox setCheck(False)', w.getCheck(chk) == 0, str(w.getCheck(chk)))
    w.setCheck(rad, True)
    record('勾选', 'RadioButton setCheck(True)', w.getCheck(rad) == BST_CHECKED, str(w.getCheck(rad)))
    record('文本', 'GroupBox getText', True, w.getText(grp))

    # 列表 / 下拉
    for name, cid in (('ComboBox', cmb), ('ListBox', lst)):
        w.clearStrings(cid)
        w.addString(cid, '甲')
        w.addString(cid, '乙')
        w.addString(cid, '丙')
        record(name, f'{name} countStrings=3', w.countStrings(cid) == 3, str(w.countStrings(cid)))
        record(name, f'{name} getString(1)', w.getString(cid, 1) == '乙', w.getString(cid, 1))
        w.setCurSel(cid, 2)
        record(name, f'{name} setCurSel/getCurSel', w.getCurSel(cid) == 2, str(w.getCurSel(cid)))
    w.deleteString(lst, 1)
    record('ListBox', 'deleteString 后 count=2', w.countStrings(lst) == 2, str(w.countStrings(lst)))

    # 进度 / 滑块
    w.setRange(prg, 0, 200)
    w.setPos(prg, 120)
    record('数值', 'ProgressBar setRange/setPos/getPos', w.getPos(prg) == 120, str(w.getPos(prg)))
    w.setRange(sld, 0, 100)
    w.setPos(sld, 42)
    record('数值', 'Slider setRange/setPos/getPos', w.getPos(sld) == 42, str(w.getPos(sld)))

    # 显示 / 禁用
    w.setVisible(hid, False)
    record('状态', 'setVisible(False)', w.getVisible(hid) is False, str(w.getVisible(hid)))
    w.setVisible(hid, True)
    record('状态', 'setVisible(True)', w.getVisible(hid) is True, str(w.getVisible(hid)))
    w.setEnabled(hid, False)
    record('状态', 'setEnabled(False)', w.getEnabled(hid) is False, str(w.getEnabled(hid)))
    w.setEnabled(hid, True)
    record('状态', 'setEnabled(True)', w.getEnabled(hid) is True, str(w.getEnabled(hid)))

    # 移动
    w.move(hid, 20, 560, 180, 30)
    hwnd = cmap['btnHidden'][1]
    r = wintypes.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    record('布局', 'move() 生效', (r.right - r.left) == 180 and (r.bottom - r.top) == 30,
           f'{r.left},{r.top} {r.right - r.left}x{r.bottom - r.top}')

    # 窗口属性
    size = w.clientSize()
    record('窗口', 'clientSize()', isinstance(size, tuple) and size[0] > 0, str(size))
    w.setTheme(True); p.pump(120)
    record('窗口', 'setTheme(dark)', True)
    w.setTheme(False); p.pump(120)
    record('窗口', 'setTheme(light)', True)


def test_events(w, cmap, names):
    section('事件回调')
    hits = {'click': 0, 'checked': 0, 'change': 0, 'select': 0, 'value': 0}

    w.onClick(names['btnOk'], lambda *a: hits.__setitem__('click', hits['click'] + 1))
    w.onChecked(names['chkAgree'], lambda *a: hits.__setitem__('checked', hits['checked'] + 1))
    w.onChange(names['txtInput'], lambda *a: hits.__setitem__('change', hits['change'] + 1))
    w.onSelect(names['lstItems'], lambda *a: hits.__setitem__('select', hits['select'] + 1))
    w.onValue(names['sldLevel'], lambda *a: hits.__setitem__('value', hits['value'] + 1))

    # 按钮点击：BM_CLICK 由控件自己发 BN_CLICKED 给父窗口
    user32.SendMessageW(cmap['btnOk'][1], BM_CLICK, 0, 0)
    p.pump(250)
    record('事件', 'onClick 触发', hits['click'] == 1, str(hits['click']))

    # 复选框点击
    user32.SendMessageW(cmap['chkAgree'][1], BM_CLICK, 0, 0)
    p.pump(250)
    record('事件', 'onChecked 触发', hits['checked'] == 1, str(hits['checked']))
    record('事件', '点击后勾选态反转', w.getCheck(names['chkAgree']) == BST_CHECKED,
           str(w.getCheck(names['chkAgree'])))


def test_extended(w, cmap, names):
    section('扩展控件行为')
    for key, label in (('lnkOpen', 'Hyperlink'), ('rtfNote', 'TextArea'),
                       ('tglMode', 'ToggleButton'), ('bbtnRun', 'IconButton')):
        cid = names[key]
        w.setText(cid, '文本-' + label)
        record('扩展', f'{label} setText/getText', w.getText(cid) == '文本-' + label, w.getText(cid))
    w.setCheck(names['tglMode'], True)
    record('扩展', 'ToggleButton setCheck', w.getCheck(names['tglMode']) == BST_CHECKED,
           str(w.getCheck(names['tglMode'])))
    w.setVisible(names['lnSep'], False)
    record('扩展', 'StaticLine setVisible', w.getVisible(names['lnSep']) is False,
           str(w.getVisible(names['lnSep'])))
    w.setVisible(names['lnSep'], True)


def test_panel():
    section('Panel 容器（外部 HWND 宿主）')
    # 造一个真实父窗口
    hinst = ctypes.windll.kernel32.GetModuleHandleW(None)
    user32.CreateWindowExW.restype = wintypes.HWND
    parent = user32.CreateWindowExW(0, 'STATIC', 'PanelHost', 0x00CF0000, 100, 100, 400, 300,
                                    0, 0, hinst, None)
    record('Pane', '父窗口创建', bool(parent), hex(parent or 0))
    if not parent:
        return
    user32.ShowWindow(parent, SW_SHOW)
    pnl = p.Panel(parent)
    record('Pane', 'Panel 创建', pnl.hwnd != 0, hex(pnl.hwnd))
    pnl.resize()
    pnl.addLabel(3001, '标签', 10, 10, 120, 24)
    pnl.addButton(3002, '按钮', 10, 40, 100, 30)
    pnl.addEdit(3003, '文本', 10, 80, 200, 28)
    pnl.addCheckBox(3004, '勾选', 10, 120, 160, 24)
    pnl.addCombo(3005, 10, 150, 160, 28)
    pnl.addListBox(3006, 10, 190, 200, 80)
    pnl.addProgress(3007, 10, 280, 200, 18)
    pnl.addSlider(3008, 0, 100, 220, 10, 200, 28)

    kids = enum_children(pnl.hwnd)
    for cid, expect in ((3001, 'Static'), (3002, 'Button'), (3003, 'Edit'), (3004, 'Button'),
                        (3005, 'ComboBox'), (3006, 'ListBox'), (3007, 'msctls_progress32')):
        got = kids.get(cid)
        record('Pane', f'控件 {cid} 类名', bool(got) and got[1] == expect,
               (got[1] if got else '缺失'))
    record('Pane', 'Slider 类名', kids.get(3008, (None, ''))[1] == 'msctls_trackbar32',
           kids.get(3008, (None, ''))[1])

    pnl.setText(3003, '面板内文本')
    record('Pane', 'setText/getText', pnl.getText(3003) == '面板内文本', pnl.getText(3003))
    pnl.setCheck(3004, True)
    record('Pane', 'setCheck', pnl.getCheck(3004) == BST_CHECKED, str(pnl.getCheck(3004)))
    pnl.setPos(3007, 50)
    record('Pane', 'setPos(progress)', pnl.getPos(3007) == 50, str(pnl.getPos(3007)))
    pnl.setVisible(3002, False)
    record('Pane', 'setVisible', pnl.getVisible(3002) is False, str(pnl.getVisible(3002)))
    user32.DestroyWindow(parent)


def test_event_extended(w, cmap, names):
    """编辑框改字 / 列表选择 / 滑块拖动的通知路径（发真实 WM_COMMAND / WM_HSCROLL）。"""
    section('事件：通知消息路径')
    hits = {'change': 0, 'select': 0, 'value': 0}
    w.onChange(names['txtInput'], lambda *a: hits.__setitem__('change', hits['change'] + 1))
    w.onSelect(names['lstItems'], lambda *a: hits.__setitem__('select', hits['select'] + 1))
    w.onValue(names['sldLevel'], lambda *a: hits.__setitem__('value', hits['value'] + 1))

    edit = cmap['txtInput'][1]
    w.setFocus(names['txtInput'])
    p.pump(100)
    user32.SendMessageW(edit, WM_CHAR, ord('Z'), 0)
    p.pump(250)
    record('事件', 'onChange 触发（模拟键入）', hits['change'] >= 1,
           f'count={hits["change"]} text={w.getText(names["txtInput"])}')

    lst = cmap['lstItems'][1]
    par = parent_of(lst)
    user32.SendMessageW(par, WM_COMMAND,
                        (names['lstItems'] & 0xFFFF) | (LBN_SELCHANGE << 16), lst)
    p.pump(250)
    record('事件', 'onSelect 触发（LBN_SELCHANGE）', hits['select'] >= 1, str(hits['select']))

    sld = cmap['sldLevel'][1]
    par2 = parent_of(sld)
    user32.SendMessageW(par2, WM_HSCROLL, (TB_THUMBTRACK << 16) | (names['sldLevel'] & 0xFFFF), sld)
    p.pump(250)
    record('事件', 'onValue 触发（WM_HSCROLL）', hits['value'] >= 1, str(hits['value']))


def main():
    print('PyMfcXaml 控件全量测试')
    print('解释器:', sys.executable)
    print('模块:', p.__file__)

    # ---------------- 基础窗口 ----------------
    w, cmap, names = run_window('controls_basic.xaml', '基础窗口', TAG_OF)
    test_basic(w, cmap, names)
    test_event_extended(w, cmap, names)
    test_events(w, cmap, names)
    w.close(); p.pump(250)

    # ---------------- 扩展窗口 ----------------
    w2, cmap2, names2 = run_window('controls_extended.xaml', '扩展窗口', TAG_OF)
    test_extended(w2, cmap2, names2)
    w2.close(); p.pump(250)

    # ---------------- Panel 容器 ----------------
    test_panel()

    p.shutdown()

    print()
    passed = sum(1 for r in results if r[2] is True)
    failed = [r for r in results if r[2] is False]
    info_count = sum(1 for r in results if r[2] is None)
    print(f'合计 {len(results)} 项：通过 {passed}，失败 {len(failed)}，信息 {info_count}')
    if failed:
        print('失败明细：')
        for s, item, ok, detail in failed:
            print(f'  [{s}] {item} :: {detail}')
        return 1
    print('CONTROLS TEST PASSED')
    return 0


if __name__ == '__main__':
    sys.exit(main())
