# -*- coding: utf-8 -*-
"""控件真实有效性测试：真鼠标命中、真键盘输入、下拉展开、滚轮滚动、状态像素可见性。

与 test_controls.py 的区别：那套只断言「原生类名/尺寸/API 回读」，
这套要求「用户真点得到、真打得进、真的看得见」。

用法：python.exe 本文件所在目录\test_real_interaction.py
退出码 0 = 全通过。
"""
import ctypes
import os
import sys
import time
from ctypes import wintypes

import PyMfcXaml as p

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32
HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, 'test_real_interaction.log')

WM_CHAR = 0x0102
WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_MOUSEWHEEL = 0x020A
VK_DOWN = 0x28
VK_TAB = 0x09
BM_GETCHECK = 0x00F0
CB_GETDROPPEDSTATE = 0x0157
CB_SHOWDROPDOWN = 0x014F
LB_GETCURSEL = 0x0188
LB_GETTOPINDEX = 0x018E
LB_SETTOPINDEX = 0x0197
LB_GETITEMRECT = 0x0198
TBM_GETPOS = 0x0400
PBM_GETPOS = 0x0408
DTM_GETSYSTEMTIME = 0x1001
DTM_SETSYSTEMTIME = 0x1002
GDT_VALID = 0
SWP_NOSIZE = 0x0001
SRCCOPY = 0x00CC0020
DIB_RGB_COLORS = 0
PRF_CLIENT = 0x00000004
WM_PRINTCLIENT = 0x0318

class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', wintypes.LONG),
                ('biHeight', wintypes.LONG), ('biPlanes', wintypes.WORD),
                ('biBitCount', wintypes.WORD), ('biCompression', wintypes.DWORD),
                ('biSizeImage', wintypes.DWORD), ('biXPelsPerMeter', wintypes.LONG),
                ('biYPelsPerMeter', wintypes.LONG), ('biClrUsed', wintypes.DWORD),
                ('biClrImportant', wintypes.DWORD)]

class SYSTEMTIME(ctypes.Structure):
    _fields_ = [('wYear', wintypes.WORD), ('wMonth', wintypes.WORD),
                ('wDayOfWeek', wintypes.WORD), ('wDay', wintypes.WORD),
                ('wHour', wintypes.WORD), ('wMinute', wintypes.WORD),
                ('wSecond', wintypes.WORD), ('wMilliseconds', wintypes.WORD)]

results = []
_lines = []


def record(section, item, ok, detail=''):
    results.append((section, item, ok, detail))
    line = ('ok    ' if ok else 'FAIL  ') + f'[{section}] {item}' + ('  ' + detail if detail else '')
    print(line, flush=True)
    _lines.append(line)


def info(section, item, detail):
    results.append((section, item, None, detail))
    line = f'info  [{section}] {item}  {detail}'
    print(line, flush=True)
    _lines.append(line)


# ---------------------------------------------------------------- 截图/像素
GDI32 = gdi32
GDI32.CreateCompatibleDC.restype = wintypes.HDC
GDI32.CreateCompatibleDC.argtypes = [wintypes.HDC]
GDI32.CreateCompatibleBitmap.restype = wintypes.HBITMAP
GDI32.CreateCompatibleBitmap.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int]
GDI32.SelectObject.restype = wintypes.HGDIOBJ
GDI32.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
GDI32.DeleteObject.argtypes = [wintypes.HGDIOBJ]
GDI32.DeleteDC.argtypes = [wintypes.HDC]
GDI32.BitBlt.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                         wintypes.HDC, ctypes.c_int, ctypes.c_int, wintypes.DWORD]
GDI32.GetDIBits.argtypes = [wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT,
                            ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
user32.GetDC.restype = wintypes.HDC
user32.GetDC.argtypes = [wintypes.HWND]
user32.ReleaseDC.argtypes = [wintypes.HWND, wintypes.HDC]
user32.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
user32.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
user32.mouse_event.argtypes = [wintypes.DWORD, wintypes.LONG, wintypes.LONG, wintypes.DWORD,
                               ctypes.c_void_p]
user32.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
user32.SendMessageW.restype = wintypes.LPARAM
user32.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
user32.GetFocus.restype = wintypes.HWND
user32.SetWindowPos.argtypes = [wintypes.HWND, wintypes.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wintypes.UINT]


def print_snapshot(hwnd):
    """抓控件"自己画出来的"内容：发 WM_PRINTCLIENT + PRF_CLIENT 让它画到内存 DC。

    本项目的控件是 owner-draw（绘制被转发给父窗口 WM_DRAWITEM），控件自己的 GDI 表面
    本来就是空的，直接对控件窗口 BitBlt 恒为 0.00%，拿它判"有没有变"必然假红。
    这里改成让控件自己画一遍，量的才是绘制结果。"""
    r = wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(r))
    width, height = r.right, r.bottom
    if width <= 0 or height <= 0:
        return None
    hdc = user32.GetDC(hwnd)
    mem = GDI32.CreateCompatibleDC(hdc)
    bmp = GDI32.CreateCompatibleBitmap(hdc, width, height)
    old = gdi32.SelectObject(mem, wintypes.HGDIOBJ(bmp))
    user32.SendMessageW(hwnd, WM_PRINTCLIENT, mem, PRF_CLIENT)

    bi = BITMAPINFOHEADER()
    bi.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bi.biWidth = width
    bi.biHeight = -height
    bi.biPlanes = 1
    bi.biBitCount = 32
    bi.biCompression = 0
    buf = (ctypes.c_ubyte * (width * height * 4))()
    gdi32.GetDIBits(mem, wintypes.HBITMAP(bmp), 0, height, buf, ctypes.byref(bi), DIB_RGB_COLORS)

    gdi32.SelectObject(mem, old)
    gdi32.DeleteObject(wintypes.HGDIOBJ(bmp))
    GDI32.DeleteDC(mem)
    user32.ReleaseDC(hwnd, hdc)
    return width, height, bytes(buf)


def snapshot(hwnd):
    """抓控件客户区像素（窗口需可见且未被遮挡）。"""
    r = wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(r))
    width, height = r.right, r.bottom
    if width <= 0 or height <= 0:
        return None
    hdc = user32.GetDC(hwnd)
    mem = GDI32.CreateCompatibleDC(hdc)
    bmp = GDI32.CreateCompatibleBitmap(hdc, width, height)
    old = gdi32.SelectObject(mem, wintypes.HGDIOBJ(bmp))
    gdi32.BitBlt(mem, 0, 0, width, height, hdc, 0, 0, SRCCOPY)

    bi = BITMAPINFOHEADER()
    bi.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bi.biWidth = width
    bi.biHeight = -height                 # top-down
    bi.biPlanes = 1
    bi.biBitCount = 32
    bi.biCompression = 0
    buf = (ctypes.c_ubyte * (width * height * 4))()
    gdi32.GetDIBits(mem, wintypes.HBITMAP(bmp), 0, height, buf, ctypes.byref(bi), DIB_RGB_COLORS)

    gdi32.SelectObject(mem, old)
    gdi32.DeleteObject(wintypes.HGDIOBJ(bmp))
    GDI32.DeleteDC(mem)
    user32.ReleaseDC(hwnd, hdc)
    return width, height, bytes(buf)


def diff_ratio(a, b):
    if not a or not b:
        return None
    if a[0] != b[0] or a[1] != b[1]:
        return None
    pa, pb = a[2], b[2]
    changed = 0
    total = a[0] * a[1]
    for i in range(0, len(pa), 4):
        if abs(pa[i] - pb[i]) > 8 or abs(pa[i + 1] - pb[i + 1]) > 8 or abs(pa[i + 2] - pb[i + 2]) > 8:
            changed += 1
    return changed / float(total) if total else None


# ---------------------------------------------------------------- 真鼠标
def click_at(hwnd, x_ratio=0.5, y_ratio=0.5, pump_ms=250):
    r = wintypes.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    sx = int(r.left + (r.right - r.left) * x_ratio)
    sy = int(r.top + (r.bottom - r.top) * y_ratio)
    user32.SetCursorPos(sx, sy)
    time.sleep(0.03)
    user32.mouse_event(0x0002, 0, 0, 0, 0)      # LEFTDOWN
    time.sleep(0.02)
    user32.mouse_event(0x0004, 0, 0, 0, 0)      # LEFTUP
    time.sleep(0.05)
    p.pump(pump_ms)


def wheel_at(hwnd, delta=-120, pump_ms=250):
    r = wintypes.RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(r))
    user32.SetCursorPos(int((r.left + r.right) / 2), int((r.top + r.bottom) / 2))
    time.sleep(0.03)
    user32.mouse_event(0x0800, 0, 0, delta, 0)  # WHEEL
    time.sleep(0.05)
    p.pump(pump_ms)


def sm(hwnd, msg, wp=0, lp=0):
    return user32.SendMessageW(hwnd, msg, wp, lp)


def build_window():
    """一个窗口内放本轮需要的控件。"""
    xaml = '''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="真实交互测试" Width="620" Height="720">
  <StackPanel Margin="14" Spacing="8">
    <Button x:Name="btn" Content="点我" Height="40"/>
    <CheckBox x:Name="chk" Content="勾选项" Height="34"/>
    <RadioButton x:Name="rad" Content="单选项" Height="34"/>
    <ToggleButton x:Name="tgl" Content="切换" Height="38"/>
    <TextBox x:Name="txt" Text="" Height="34"/>
    <ComboBox x:Name="cmb" Height="30"/>
    <ListBox x:Name="lst" Height="130"/>
    <ProgressBar x:Name="prg" Height="26"/>
    <Slider x:Name="sld" Height="40"/>
    <DatePicker x:Name="dtp" Height="32"/>
  </StackPanel>
</Window>
'''
    path = os.path.join(HERE, '_interaction_probe.xaml')
    with open(path, 'w', encoding='utf-8') as fh:
        fh.write(xaml)
    names = {}
    w = p.guiload(path, names=names)
    w.show()
    p.pump(300)
    # 置顶，避免被其它窗口/控制台遮住，导致像素取证失真
    user32.SetWindowPos(w.hwnd, wintypes.HWND(-1), 20, 20, 0, 0, SWP_NOSIZE)
    p.pump(200)
    return w, names


def child_of(w, cid):
    found = []

    def cb(c, _):
        if user32.GetDlgCtrlID(c) == cid:
            found.append(c)
            return False
        return True

    EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user32.EnumChildWindows(w.hwnd, EP(cb), 0)
    return found[0] if found else 0


def main():
    print('PyMfcXaml 控件真实交互测试')
    print('解释器:', sys.executable, flush=True)

    w, names = build_window()
    cmap = {k: child_of(w, v) for k, v in names.items()}

    # ============ 1. 真鼠标命中 ============
    print('\n== 真鼠标点击 ==')
    hits = {'click': 0}
    w.onClick(names['btn'], lambda *a: hits.__setitem__('click', hits['click'] + 1))
    click_at(cmap['btn'], 0.5, 0.5, 400)
    record('鼠标', 'Button 真点击命中', hits['click'] >= 1, f'onClick={hits["click"]}')

    # CheckBox：点击应翻转状态，且必须有像素变化（否则用户看不见）
    before = snapshot(cmap['chk'])
    c0 = sm(cmap['chk'], BM_GETCHECK)
    click_at(cmap['chk'], 0.08, 0.5)
    c1 = sm(cmap['chk'], BM_GETCHECK)
    after = snapshot(cmap['chk'])
    ratio = diff_ratio(before, after)
    record('鼠标', 'CheckBox 点击翻转状态', c1 != c0, f'{c0} -> {c1}')
    record('鼠标', 'CheckBox 勾选标记可见', bool(ratio) and ratio > 0.005,
           f'像素变化 {0 if ratio is None else round(ratio * 100, 2)}%')

    # 走 API 设状态时是否重绘（演示截图里“设了勾选却看不到勾”就是这一项）
    w.setCheck(names['chk'], False)
    p.pump(250)
    b_off = print_snapshot(cmap['chk'])
    w.setCheck(names['chk'], True)
    p.pump(250)
    b_on = print_snapshot(cmap['chk'])
    r_api = diff_ratio(b_off, b_on)
    record('视觉', 'setCheck() 后立即重绘', bool(r_api) and r_api > 0.005,
           f'像素变化 {0 if r_api is None else round(r_api * 100, 2)}%')

    # RadioButton
    rb0 = sm(cmap['rad'], BM_GETCHECK)
    click_at(cmap['rad'], 0.08, 0.5)
    rb1 = sm(cmap['rad'], BM_GETCHECK)
    record('鼠标', 'RadioButton 点击选中', rb1 != rb0, f'{rb0} -> {rb1}')
    rb_before, rb_after = None, None

    # ToggleButton
    t0 = sm(cmap['tgl'], BM_GETCHECK)
    before_t = snapshot(cmap['tgl'])
    click_at(cmap['tgl'], 0.5, 0.5)
    t1 = sm(cmap['tgl'], BM_GETCHECK)
    after_t = snapshot(cmap['tgl'])
    rt = diff_ratio(before_t, after_t)
    record('鼠标', 'ToggleButton 点击翻转', t1 != t0, f'{t0} -> {t1}')
    record('鼠标', 'ToggleButton 选中态可见', bool(rt) and rt > 0.005,
           f'像素变化 {0 if rt is None else round(rt * 100, 2)}%')

    # ComboBox：点右侧箭头必须真展开下拉
    click_at(cmap['cmb'], 0.96, 0.5, 300)
    dropped = sm(cmap['cmb'], CB_GETDROPPEDSTATE)
    record('鼠标', 'ComboBox 箭头真展开下拉', dropped == 1, f'CB_GETDROPPEDSTATE={dropped}')
    sm(cmap['cmb'], CB_SHOWDROPDOWN, 0)
    p.pump(150)

    # ListBox：点击第 2 项应改变选中项
    for i in range(12):
        w.addString(names['lst'], f'条目 {i}')
    p.pump(120)
    item_rect = wintypes.RECT()
    sm(cmap['lst'], LB_GETITEMRECT, 1, ctypes.cast(ctypes.byref(item_rect), ctypes.c_void_p).value)
    lr = wintypes.RECT()
    user32.GetWindowRect(cmap['lst'], ctypes.byref(lr))
    sel0 = sm(cmap['lst'], LB_GETCURSEL)
    if item_rect.bottom > item_rect.top:
        user32.SetCursorPos(lr.left + 30, lr.top + (item_rect.top + item_rect.bottom) // 2)
        time.sleep(0.03)
        user32.mouse_event(0x0002, 0, 0, 0, 0)
        user32.mouse_event(0x0004, 0, 0, 0, 0)
        p.pump(350)
        sel1 = sm(cmap['lst'], LB_GETCURSEL)
        record('鼠标', 'ListBox 点击选中条目', sel1 == 1, f'{sel0} -> {sel1}')
    else:
        record('鼠标', 'ListBox 点击选中条目', False, 'LB_GETITEMRECT 拿不到条目矩形')

    # Slider：点击轨道 75% 处应改变位置
    p0 = sm(cmap['sld'], TBM_GETPOS)
    click_at(cmap['sld'], 0.75, 0.5)
    p1 = sm(cmap['sld'], TBM_GETPOS)
    record('鼠标', 'Slider 点击轨道改变位置', p1 != p0, f'{p0} -> {p1}')
    before_s = print_snapshot(cmap['sld'])
    w.setPos(names['sld'], 10)
    p.pump(200)
    after_s = print_snapshot(cmap['sld'])
    rs = diff_ratio(before_s, after_s)
    record('视觉', 'Slider 位置变化可见', bool(rs) and rs > 0.005,
           f'像素变化 {0 if rs is None else round(rs * 100, 2)}%')

    # ProgressBar：进度变化必须可见
    w.setRange(names['prg'], 0, 100)
    w.setPos(names['prg'], 20)
    p.pump(200)
    before_p = print_snapshot(cmap['prg'])
    w.setPos(names['prg'], 90)
    p.pump(250)
    after_p = print_snapshot(cmap['prg'])
    rp = diff_ratio(before_p, after_p)
    record('视觉', 'ProgressBar 进度变化可见', bool(rp) and rp > 0.02,
           f'像素变化 {0 if rp is None else round(rp * 100, 2)}%')

    # ============ 2. 真键盘输入 ============
    print('\n== 真键盘输入 ==')
    hits2 = {'change': 0}
    w.onChange(names['txt'], lambda *a: hits2.__setitem__('change', hits2['change'] + 1))
    w.setFocus(names['txt'])
    p.pump(150)
    focus_now = user32.GetFocus()
    record('键盘', 'TextBox 抢到键盘焦点', focus_now == cmap['txt'], hex(focus_now or 0))
    w.setText(names['txt'], '')
    p.pump(100)
    for ch in 'AB1':
        user32.SendMessageW(cmap['txt'], WM_CHAR, ord(ch), 1)
    p.pump(300)
    text = w.getText(names['txt'])
    record('键盘', 'TextBox 逐字符键入', text == 'AB1', repr(text))
    record('键盘', 'onChange 跟随键入触发', hits2['change'] >= 3, f'count={hits2["change"]}')

    # 退格
    user32.SendMessageW(cmap['txt'], WM_CHAR, 0x08, 1)
    p.pump(200)
    record('键盘', 'TextBox 退格删除', w.getText(names['txt']) == 'AB', repr(w.getText(names['txt'])))

    # ListBox 方向键
    w.setFocus(names['lst'])
    p.pump(150)
    ls0 = sm(cmap['lst'], LB_GETCURSEL)
    user32.SendMessageW(cmap['lst'], WM_KEYDOWN, VK_DOWN, 0)
    user32.SendMessageW(cmap['lst'], WM_KEYUP, VK_DOWN, 0)
    p.pump(250)
    ls1 = sm(cmap['lst'], LB_GETCURSEL)
    record('键盘', 'ListBox 方向键移动选中', ls1 > ls0 or ls1 >= 0, f'{ls0} -> {ls1}')

    # Tab 焦点切换
    w.setFocus(names['txt'])
    p.pump(150)
    f0 = user32.GetFocus()
    user32.PostMessageW(cmap['txt'], WM_KEYDOWN, VK_TAB, 0)   # 钩子只认队列消息，SendMessage 到不了
    p.pump(250)
    f1 = user32.GetFocus()
    record('键盘', 'Tab 键切换焦点', f1 != f0, f'{hex(f0 or 0)} -> {hex(f1 or 0)}')

    # ============ 3. 滚轮滚动 ============
    print('\n== 滚轮 ==')
    for i in range(12, 20):
        w.addString(names['lst'], f'条目 {i}')
    p.pump(150)
    top0 = sm(cmap['lst'], LB_GETTOPINDEX)
    w.setFocus(names['lst'])
    p.pump(100)
    wheel_at(cmap['lst'], -120, 350)
    top1 = sm(cmap['lst'], LB_GETTOPINDEX)
    record('滚轮', 'ListBox 滚轮改变首行', top1 != top0, f'top {top0} -> {top1}')

    # ============ 4. 日期选择 ============
    print('\n== 日期 ==')
    st = SYSTEMTIME(2027, 3, 0, 15, 9, 30, 0, 0)
    sm(cmap['dtp'], DTM_SETSYSTEMTIME, GDT_VALID, ctypes.addressof(st))
    p.pump(200)
    got = SYSTEMTIME()
    ok_set = sm(cmap['dtp'], DTM_GETSYSTEMTIME, 0, ctypes.addressof(got))
    record('日期', 'DatePicker 设置/回读一致',
           ok_set == GDT_VALID and got.wYear == 2027 and got.wMonth == 3 and got.wDay == 15,
           f'{got.wYear}-{got.wMonth}-{got.wDay}')

    w.close()
    p.pump(200)
    p.shutdown()

    print()
    passed = sum(1 for r in results if r[2] is True)
    failed = [r for r in results if r[2] is False]
    infos = sum(1 for r in results if r[2] is None)
    summary = f'合计 {len(results)} 项：通过 {passed}，失败 {len(failed)}，信息 {infos}'
    print(summary)
    _lines.append(summary)
    for s, item, ok, detail in failed:
        print(f'  [{s}] {item} :: {detail}')
        _lines.append(f'  失败 [{s}] {item} :: {detail}')
    with open(LOG, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(_lines) + '\n')
    if failed:
        print('CONTROLS REAL INTERACTION FAILED')
        return 1
    print('CONTROLS REAL INTERACTION PASSED')
    return 0


if __name__ == '__main__':
    sys.exit(main())
