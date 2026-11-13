# -*- coding: utf-8 -*-
"""控件功能齐全度审计：区分「只有样子」与「真有功能」。

检查两类事实：
1) 接口面：PyMfcXaml 对 Python 暴露了哪些内容/交互能力（缺的即“装不进去”）；
2) 行为面：链接点击、分隔条拖拽、滚动、日历选日、富文本格式、位图按钮、下拉展开回调。

用法：python.exe 本目录\audit_functional.py
"""
import ctypes
import os
import sys
import time
from ctypes import wintypes

import PyMfcXaml as p

user32 = ctypes.windll.user32
HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, 'audit_functional.log')

BM_GETIMAGE = 0x00F6
IMAGE_BITMAP = 0
TCM_GETITEMCOUNT = 0x1304
TVM_GETNEXTITEM = 0x110A
TVGN_ROOT = 0
TVGN_CHILD = 4
LVM_GETITEMCOUNT = 0x1004
LVM_GETHEADER = 0x101F
HDM_GETITEMCOUNT = 0x1200
MCM_GETCURSEL = 0x1001
MCM_SETCURSEL = 0x1002
EM_SETCHARFORMAT = 0x0444
EM_GETCHARFORMAT = 0x043A
SCF_SELECTION = 0x0001
SCF_ALL = 0x0004
CFM_BOLD = 0x00000001
CFE_BOLD = 0x00000001
WS_VSCROLL = 0x00200000
SB_VERT = 1
SWP_NOSIZE = 0x0001
SWP_SHOWWINDOW = 0x0040
CB_GETDROPPEDSTATE = 0x0157


class SYSTEMTIME(ctypes.Structure):
    _fields_ = [('wYear', wintypes.WORD), ('wMonth', wintypes.WORD),
                ('wDayOfWeek', wintypes.WORD), ('wDay', wintypes.WORD),
                ('wHour', wintypes.WORD), ('wMinute', wintypes.WORD),
                ('wSecond', wintypes.WORD), ('wMilliseconds', wintypes.WORD)]


class CHARFORMATW(ctypes.Structure):
    _fields_ = [('cbSize', wintypes.UINT), ('dwMask', wintypes.DWORD),
                ('dwEffects', wintypes.DWORD), ('yHeight', wintypes.LONG),
                ('yOffset', wintypes.LONG), ('crTextColor', wintypes.DWORD),
                ('bCharSet', wintypes.BYTE), ('bPitchAndFamily', wintypes.BYTE),
                ('szFaceName', wintypes.WCHAR * 32), ('wWeight', wintypes.WORD),
                ('sSpacing', ctypes.c_short), ('crBackColor', wintypes.DWORD),
                ('lcid', wintypes.DWORD), ('dwReserved', wintypes.DWORD),
                ('sStyle', ctypes.c_short), ('wKerning', wintypes.WORD),
                ('bUnderlineType', wintypes.BYTE), ('bAnimation', wintypes.BYTE),
                ('bRevAuthor', wintypes.BYTE), ('bReserved1', wintypes.BYTE)]


class SCROLLINFO(ctypes.Structure):
    _fields_ = [('cbSize', wintypes.UINT), ('fMask', wintypes.UINT),
                ('nMin', ctypes.c_int), ('nMax', ctypes.c_int),
                ('nPage', wintypes.UINT), ('nPos', ctypes.c_int),
                ('nTrackPos', ctypes.c_int)]


results = []
lines = []


def record(section, item, ok, detail=''):
    results.append((section, item, ok, detail))
    line = ('ok    ' if ok else 'FAIL  ') + f'[{section}] {item}' + ('  ' + detail if detail else '')
    print(line, flush=True)
    lines.append(line)


def info(section, item, detail):
    results.append((section, item, None, detail))
    line = f'info  [{section}] {item}  {detail}'
    print(line, flush=True)
    lines.append(line)


user32.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
user32.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user32.SendMessageW.restype = wintypes.LPARAM
user32.GetWindowLongPtrW.argtypes = [wintypes.HWND, ctypes.c_int]
user32.GetWindowLongPtrW.restype = ctypes.c_ssize_t
user32.SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wintypes.UINT]
user32.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
user32.GetScrollInfo.argtypes = [wintypes.HWND, ctypes.c_int, ctypes.POINTER(SCROLLINFO)]
user32.mouse_event.argtypes = [wintypes.DWORD, wintypes.LONG, wintypes.LONG, wintypes.DWORD,
                              ctypes.c_void_p]
user32.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
user32.GetDlgCtrlID.argtypes = [ctypes.c_void_p]


def child_of(top, cid):
    found = []

    def cb(c, _):
        if user32.GetDlgCtrlID(c) == cid:
            found.append(c)
            return False
        return True

    EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user32.EnumChildWindows(top, EP(cb), 0)
    return found[0] if found else 0


def rect_of(h):
    r = wintypes.RECT()
    user32.GetWindowRect(h, ctypes.byref(r))
    return (r.left, r.top, r.right - r.left, r.bottom - r.top)


def click(hwnd, x=0.5, y=0.5, pump_ms=300):
    x0, y0, w, h = rect_of(hwnd)
    user32.SetCursorPos(int(x0 + w * x), int(y0 + h * y))
    time.sleep(0.03)
    user32.mouse_event(0x0002, 0, 0, 0, 0)
    time.sleep(0.02)
    user32.mouse_event(0x0004, 0, 0, 0, 0)
    time.sleep(0.05)
    p.pump(pump_ms)


def api_surface():
    print('== 接口面 ==')
    names = [n for n in dir(p.Window) if not n.startswith('_')]
    info('接口', 'pybind 暴露的方法数', str(len(names)))
    adders = [n for n in names if n.startswith('add')]
    info('接口', '内容构造类接口', ', '.join(adders))
    missing = [n for n in ('addTab', 'addTabItem', 'addTree', 'addTreeItem', 'addListCtrl',
                           'addColumn', 'addRow', 'addGrid', 'addImage', 'addBitmap',
                           'setImage', 'setBitmap', 'setIcon', 'setFont', 'setStyle',
                           'addCalendar', 'addDatePicker', 'addChoice', 'addPanel',
                           'addSplitter', 'addStaticLine', 'addHyperlink', 'addRichText',
                           'addToggle', 'addBitmapButton', 'addStaticBitmap', 'addGridCtrl',
                           'addScrolled', 'addCalendarCtrl')
               if n not in names]
    info('接口', '缺失的内容/样式接口', ', '.join(missing))


def build_window():
    xaml = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="功能审计" Width="560" Height="760">
  <XStackPanel Margin="14" Spacing="8">
    <XHyperlink x:Name="lnk" Text="打开示例链接" Url="https://example.com" Height="30"/>
    <XSplitView x:Name="spl" Height="20"/>
    <XTextField x:Name="below" Text="分隔条下方内容" Height="40"/>
    <XScrollView x:Name="scr" ScrollMin="0" ScrollMax="500" Height="90"/>
    <XMonthGrid x:Name="cal" Height="160"/>
    <XTextArea x:Name="rtf" Height="110"/>
    <XIconButton x:Name="bb" Text="运行" Height="46"/>
    <XComboBox x:Name="cmb" Height="30"/>
  </XStackPanel>
</XWindow>
'''
    path = os.path.join(HERE, '_audit_probe.xaml')
    with open(path, 'w', encoding='utf-8') as fh:
        fh.write(xaml)
    names = {}
    w = p.guiload(path, names=names)
    w.show()
    p.pump(350)
    user32.SetWindowPos(w.hwnd, wintypes.HWND(-1), 30, 30, 560, 760, SWP_SHOWWINDOW)
    user32.SetForegroundWindow(w.hwnd)
    p.pump(300)
    return w, names


SECTIONS = []


def sec(t):
    SECTIONS.append(t)
    print('\n== ' + t + ' ==', flush=True)
    lines.append('')
    lines.append('== ' + t + ' ==')


def main():
    print('PyMfcXaml 控件功能齐全度审计')
    api_surface()

    w, names = build_window()
    cmap = {k: child_of(w.hwnd, v) for k, v in names.items()}

    # ---- Hyperlink：真点，是否有回调/是否真能当链接用 ----
    sec('Hyperlink')
    hits = {'click': 0}
    w.onClick(names['lnk'], lambda *a: hits.__setitem__('click', hits['click'] + 1))
    click(cmap['lnk'], 0.12, 0.5)   # 链接只响应文字那一小段，点太靠右等于点空白
    record('Hyperlink', '真点击触发 onClick', hits['click'] >= 1, f'onClick={hits["click"]}')
    record('Hyperlink', '可设置链接地址（URL）接口',
           any(n in dir(p.Window) for n in ('setUrl', 'setHref', 'setLink')),
           f'setUrl={hasattr(w, "setUrl")} getUrl={w.getUrl(names["lnk"]) if hasattr(w, "getUrl") else "无"}')

    # ---- Splitter：真拖，邻居是否跟随 ----
    sec('SplitView')
    before = rect_of(cmap['below'])
    x0, y0, sw, sh = rect_of(cmap['spl'])
    user32.SetCursorPos(x0 + sw // 2, y0 + sh // 2)
    time.sleep(0.1)
    user32.mouse_event(0x0002, 0, 0, 0, 0)
    time.sleep(0.05)
    p.pump(150)                     # 鼠标消息必须靠 pump 分发，不 pump 拖不动
    for dy in (10, 25, 45):
        user32.mouse_event(0x0001, 0, dy, 0, 0)
        time.sleep(0.04)
        p.pump(80)
    user32.mouse_event(0x0004, 0, 0, 0, 0)
    p.pump(350)
    after = rect_of(cmap['below'])
    record('SplitView', '拖拽改变相邻控件布局', before != after,
           f'下方控件 {before} -> {after}')

    # ---- ScrolledWindow：滚动条由皮肤自绘，范围走自有接口 ----
    sec('ScrollView')
    style = user32.GetWindowLongPtrW(cmap['scr'], -16)
    has_vscroll = bool(style & WS_VSCROLL)
    record('ScrollView', '自绘滚动条（不挂系统滚动条）', not has_vscroll, f'style=0x{style:X}')
    rng = w.getScrollRange(names['scr'])
    record('ScrollView', '存在可滚动范围', rng[1] > rng[0], f'min={rng[0]} max={rng[1]}')

    # ---- Calendar：真点某天，选中日期是否变 ----
    sec('MonthGrid')
    st = SYSTEMTIME()
    cur = user32.SendMessageW(cmap['cal'], MCM_GETCURSEL, 0, ctypes.addressof(st))
    setv = SYSTEMTIME(2026, 11, 0, 20, 0, 0, 0, 0)
    r_set = user32.SendMessageW(cmap['cal'], MCM_SETCURSEL, 0, ctypes.addressof(setv))
    p.pump(150)
    mid = SYSTEMTIME()
    user32.SendMessageW(cmap['cal'], MCM_GETCURSEL, 0, ctypes.addressof(mid))
    record('MonthGrid', 'MCM_SETCURSEL 可设日期', r_set == 1 and mid.wDay == 20,
           f'{st.wYear}-{st.wMonth}-{st.wDay} -> {mid.wMonth}-{mid.wDay}')
    click(cmap['cal'], 0.2, 0.75)
    got = SYSTEMTIME()
    r = user32.SendMessageW(cmap['cal'], MCM_GETCURSEL, 0, ctypes.addressof(got))
    record('MonthGrid', '真点击选日生效', r == 1 and got.wDay != mid.wDay,
           f'{mid.wMonth}-{mid.wDay} -> {got.wMonth}-{got.wDay}')

    # ---- RichText：原生格式能力 vs 是否暴露 ----
    sec('TextArea')
    w.setText(names['rtf'], '富文本格式测试')
    p.pump(150)
    cf = CHARFORMATW()
    cf.cbSize = ctypes.sizeof(CHARFORMATW)
    cf.dwMask = CFM_BOLD
    cf.dwEffects = CFE_BOLD
    user32.SendMessageW(cmap['rtf'], EM_SETCHARFORMAT, SCF_ALL, ctypes.addressof(cf))
    p.pump(120)
    read = CHARFORMATW()
    read.cbSize = ctypes.sizeof(CHARFORMATW)
    user32.SendMessageW(cmap['rtf'], EM_GETCHARFORMAT, SCF_ALL, ctypes.addressof(read))
    record('TextArea', '控件原生支持加粗（Bold）', bool(read.dwEffects & CFE_BOLD),
           f'dwEffects=0x{read.dwEffects:X}')
    record('TextArea', 'Python 接口暴露格式设置',
           hasattr(w, 'setRichFormat'),
           f'setRichFormat={hasattr(w, "setRichFormat")}')
    if hasattr(w, 'setRichFormat'):
        w.setRichFormat(names['rtf'], 14, True, False, 0x1565C0)
        p.pump(150)
        got = CHARFORMATW()
        got.cbSize = ctypes.sizeof(CHARFORMATW)
        user32.SendMessageW(cmap['rtf'], EM_GETCHARFORMAT, SCF_ALL, ctypes.addressof(got))
        record('TextArea', '格式真落到文本（14pt + 加粗）',
               abs(got.yHeight - 280) <= 1 and bool(got.dwEffects & CFE_BOLD),
               f'yHeight={got.yHeight} dwEffects=0x{got.dwEffects:X}')

    # ---- BitmapButton：到底有位图吗（用 setImage 真装一张）----
    sec('IconButton')
    png = os.path.join(HERE, 'res', 'dot.png')
    if hasattr(w, 'setImage'):
        loaded = w.setImage(names['bb'], png)
        p.pump(200)
        img = user32.SendMessageW(cmap['bb'], BM_GETIMAGE, IMAGE_BITMAP, 0)
        record('IconButton', '真装载了位图', loaded and bool(img),
               f'setImage={loaded} BM_GETIMAGE={img}')
    else:
        record('IconButton', '真装载了位图', False, 'Python 侧无 setImage')
    w.setText(names['bb'], '运行')
    record('IconButton', 'setText 后能显示文字', w.getText(names['bb']) == '运行', w.getText(names['bb']))

    # ---- ComboBox 下拉回调 ----
    sec('ComboBox 回调')
    drops = {'d': 0}
    if hasattr(w, 'onDropDown'):
        w.onDropDown(names['cmb'], lambda *a: drops.__setitem__('d', drops['d'] + 1))
        click(cmap['cmb'], 0.96, 0.5)
        record('ComboBox', '真展开下拉并触发 onDropDown', drops['d'] >= 1 or
               user32.SendMessageW(cmap['cmb'], CB_GETDROPPEDSTATE, 0, 0) == 1,
               f'onDropDown={drops["d"]}')
    else:
        record('ComboBox', '存在 onDropDown 接口', False, '无')

    # ---- Tab / Tree / ListCtrl / GridCtrl：内容接口 + 真装内容 ----
    sec('结构性控件')
    # 以前这一段是把结论写死的 False（快照），所以修好了也会一直报失败。
    # 现在改成真调用接口 + 读原生消息核对，避免"假红"和"假绿"。
    content_id = 9001

    # TabControl：加两个页签
    if all(hasattr(w, a) for a in ('addTab', 'addTabItem')):
        w.addTab(content_id, 10, 10, 300, 120)
        w.addTabItem(content_id, '常规')
        w.addTabItem(content_id, '高级')
        p.pump(200)
        h = child_of(w.hwnd, content_id)
        cnt = user32.SendMessageW(h, TCM_GETITEMCOUNT, 0, 0)
        record('TabBar', '装得进页签', cnt == 2, f'TCM_GETITEMCOUNT={cnt}')
        user32.DestroyWindow(h)
    else:
        record('TabBar', '具备内容接口（addTab/addTabItem）', False, 'Python 侧无对应接口')

    # TreeCtrl：父 + 子两个节点
    if all(hasattr(w, a) for a in ('addTree', 'addTreeNode')):
        w.addTree(content_id, 10, 10, 300, 120)
        w.addTreeNode(content_id, '根节点')
        w.addTreeNode(content_id, '子节点', 0)
        p.pump(200)
        h = child_of(w.hwnd, content_id)
        root = user32.SendMessageW(h, TVM_GETNEXTITEM, TVGN_ROOT, 0)
        child = user32.SendMessageW(h, TVM_GETNEXTITEM, TVGN_CHILD, root) if root else 0
        record('TreeView', '装得进节点且层级正确', bool(root) and bool(child),
               f'root={root} child={child}')
        user32.DestroyWindow(h)
    else:
        record('TreeView', '具备内容接口（addTree/addTreeNode）', False, 'Python 侧无对应接口')

    # ListCtrl / GridCtrl：列 + 行
    for label, maker, adder in (('ListCtrl', 'addListCtrl', 'addColumn'), ('GridLayout', 'addGrid', 'addColumn')):
        if all(hasattr(w, a) for a in (maker, adder, 'addRow')):
            getattr(w, maker)(content_id, 10, 10, 320, 110)
            w.addColumn(content_id, '名称', 150)
            w.addColumn(content_id, '状态', 90)
            w.addRow(content_id, '甲\t完成')
            w.addRow(content_id, '乙\t待办')
            p.pump(200)
            h = child_of(w.hwnd, content_id)
            rows = user32.SendMessageW(h, LVM_GETITEMCOUNT, 0, 0)
            cols = user32.SendMessageW(user32.SendMessageW(h, LVM_GETHEADER, 0, 0), HDM_GETITEMCOUNT, 0, 0)
            record(label, '装得进列与行', rows == 2 and cols == 2, f'columns={cols} rows={rows}')
            user32.DestroyWindow(h)
        else:
            record(label, f'具备内容接口（{maker}/{adder}）', False, 'Python 侧无对应接口')

    # StaticBitmap / BitmapButton：图片接口（这一批还没做）
    for label, exists in (('Image', ('setImage', 'addStaticBitmap')),
                          ('IconButton', ('setImage', 'addBitmapButton'))):
        ok = all(hasattr(w, a) for a in exists)
        record(label, '具备图片装载接口（setImage）', ok,
               '有 setImage' if ok else 'Python 侧没有 setImage，装不了图')

    w.close()
    p.pump(200)
    p.shutdown()

    print()
    passed = sum(1 for r in results if r[2] is True)
    failed = [r for r in results if r[2] is False]
    infos = sum(1 for r in results if r[2] is None)
    print(f'合计 {len(results)} 项：通过 {passed}，失败 {len(failed)}，信息 {infos}')
    lines.append(f'\n合计 {len(results)} 项：通过 {passed}，失败 {len(failed)}，信息 {infos}')
    with open(LOG, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(lines) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
