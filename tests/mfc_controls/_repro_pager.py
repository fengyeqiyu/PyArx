# -*- coding: utf-8 -*-
"""_repro_pager.py — 复现"两页切换后回不到初始状态"，并在本进程内抓三张图。

用 PagerRepro 窗口：StackView 两页（第 1 页 TextBlock、第 2 页 QCanvas）。
抓图口径与 _grab_title.py 相同：父窗 PrintWindow 打底 + 每个子窗 WM_PRINTCLIENT 合成。
"""
import ctypes
import os
from ctypes import wintypes

import PyMfcXaml as p

u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.MapWindowPoints.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.PrintWindow.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
g.SetViewportOrgEx.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_void_p]
g.CreateCompatibleDC.argtypes = [ctypes.c_void_p]
g.CreateCompatibleDC.restype = ctypes.c_void_p
g.CreateDIBSection.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD,
                               ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD]
g.CreateDIBSection.restype = ctypes.c_void_p
g.SelectObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.SelectObject.restype = ctypes.c_void_p
u.GetDC.argtypes = [ctypes.c_void_p]
u.GetDC.restype = ctypes.c_void_p
g.BitBlt.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                     ctypes.c_void_p, ctypes.c_int, ctypes.c_int, wintypes.DWORD]

WM_PRINTCLIENT = 0x0318
PRF_CLIENT = 0x00000004


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


class BMIH(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                ('biPlanes', wintypes.WORD), ('biBitCount', wintypes.WORD),
                ('biCompression', wintypes.DWORD), ('biSizeImage', wintypes.DWORD),
                ('biXPelsPerMeter', ctypes.c_long), ('biYPelsPerMeter', ctypes.c_long),
                ('biClrUsed', wintypes.DWORD), ('biClrImportant', wintypes.DWORD)]


EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def capture(hwnd, out):
    wr = R()
    u.GetWindowRect(hwnd, ctypes.byref(wr))
    ww, hh = wr.r - wr.l, wr.b - wr.t
    bi = BMIH()
    bi.biSize = ctypes.sizeof(BMIH)
    bi.biWidth, bi.biHeight, bi.biPlanes, bi.biBitCount = ww, -hh, 1, 32
    bits = ctypes.c_void_p()
    bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
    mdc = g.CreateCompatibleDC(0)
    g.SelectObject(mdc, bmp)
    u.PrintWindow(hwnd, mdc, 1)
    kids = []

    def on_child(h, _):
        kids.append(h)
        return True

    u.EnumChildWindows(hwnd, EP(on_child), 0)
    for h in kids:
        if not u.IsWindowVisible(h):
            continue
        cr = R()
        u.GetWindowRect(h, ctypes.byref(cr))
        pt = R()
        pt.l, pt.t = cr.l, cr.t
        u.MapWindowPoints(None, hwnd, ctypes.byref(pt), 2)
        g.SetViewportOrgEx(mdc, pt.l, pt.t, None)
        u.SendMessageW(h, WM_PRINTCLIENT, mdc, PRF_CLIENT)
    g.SetViewportOrgEx(mdc, 0, 0, None)

    data = ctypes.string_at(bits.value, ww * hh * 4)
    bi2 = BMIH()
    bi2.biSize = ctypes.sizeof(BMIH)
    bi2.biWidth, bi2.biHeight, bi2.biPlanes, bi2.biBitCount, bi2.biCompression = ww, -hh, 1, 32, 0
    bi2.biSizeImage = ww * hh * 4
    with open(out, 'wb') as f:
        f.write(b'BM')
        f.write((14 + ctypes.sizeof(BMIH) + len(data)).to_bytes(4, 'little'))
        f.write(b'\x00\x00\x00\x00')
        f.write((14 + ctypes.sizeof(BMIH)).to_bytes(4, 'little'))
        f.write(bytes(bi2))
        f.write(data)
    print('bmp %s %dx%d kids=%d' % (out, ww, hh, len(kids)))


def capture_screen(hwnd, out):
    """抓屏幕上的真实像素（不是 WM_PRINTCLIENT 合成），比对"眼前看到的"和"离屏合成的"。"""
    wr = R()
    u.GetWindowRect(hwnd, ctypes.byref(wr))
    ww, hh = wr.r - wr.l, wr.b - wr.t
    bi = BMIH()
    bi.biSize = ctypes.sizeof(BMIH)
    bi.biWidth, bi.biHeight, bi.biPlanes, bi.biBitCount = ww, -hh, 1, 32
    bits = ctypes.c_void_p()
    bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
    src = u.GetDC(0)
    mdc = g.CreateCompatibleDC(0)
    g.SelectObject(mdc, bmp)
    g.BitBlt(mdc, 0, 0, ww, hh, src, wr.l, wr.t, 0x00CC0020)
    data = ctypes.string_at(bits.value, ww * hh * 4)
    bi2 = BMIH()
    bi2.biSize = ctypes.sizeof(BMIH)
    bi2.biWidth, bi2.biHeight, bi2.biPlanes, bi2.biBitCount, bi2.biCompression = ww, -hh, 1, 32, 0
    bi2.biSizeImage = ww * hh * 4
    with open(out, 'wb') as f:
        f.write(b'BM')
        f.write((14 + ctypes.sizeof(BMIH) + len(data)).to_bytes(4, 'little'))
        f.write(b'\x00\x00\x00\x00')
        f.write((14 + ctypes.sizeof(BMIH)).to_bytes(4, 'little'))
        f.write(bytes(bi2))
        f.write(data)
    print('screen %s %dx%d' % (out, ww, hh))


XAML = r'''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="PagerRepro" Width="400" Height="320">
  <Canvas>
    <StackView x:Name="sv" Width="360" Height="200" Canvas.Left="14" Canvas.Top="14" />
    <TextBlock x:Name="pg1" Text="第一页：文字" Width="360" Height="150" Canvas.Left="14" Canvas.Top="14" />
    <Button x:Name="pb1" Content="页内按钮" Width="120" Height="36" Canvas.Left="40" Canvas.Top="60" />
    <QCanvas x:Name="pg2" Width="360" Height="150" Canvas.Left="14" Canvas.Top="14"
             Commands="Clear #313842; Rect 12,12,90,40,#00A8E8; Text 12,96,第二页:画命令,9,#FFFFFF" />
    <TextBlock x:Name="hint" Text="hint" Width="120" Height="20" Canvas.Left="14" Canvas.Top="240" />
  </Canvas>
</Window>'''


def main():
    path = r'G:\PyArx\tests\mfc_controls\_pager_repro.xaml'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XAML)
    w = p.Window('PagerRepro', 400, 320)
    names = {}
    p.guiload(path, w, globals(), names)
    sv = names['sv']
    w.addPage(sv, [names['pg1'], names['pb1']])
    w.addPage(sv, [names['pg2']])
    w.setViewIndex(sv, 0)
    w.show()
    p.pump(300)

    out_dir = r'G:\PyArx\docs'
    capture(w.hwnd, os.path.join(out_dir, '_pg_a_initial.bmp'))
    capture_screen(w.hwnd, os.path.join(out_dir, '_pg_a_initial_s.bmp'))

    w.setViewIndex(sv, 1)
    p.pump(300)
    capture(w.hwnd, os.path.join(out_dir, '_pg_b_page2.bmp'))
    capture_screen(w.hwnd, os.path.join(out_dir, '_pg_b_page2_s.bmp'))

    # 用真鼠标消息点页码条左三分之一 = 上一页，验证"点回去"这条路径
    u.GetDlgItem.argtypes = [ctypes.c_void_p, ctypes.c_int]
    u.GetDlgItem.restype = ctypes.c_void_p
    u.GetClientRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    # 控件挂在 Window 里的宿主子窗下面，不是 Window 的直接子窗：先找到能查到该 id 的那一层
    first = []

    def on_l1(h, _):
        first.append(h)
        return True

    u.EnumChildWindows(w.hwnd, EP(on_l1), 0)
    hsv = None
    for c in first:
        h = u.GetDlgItem(c, sv)
        if h:
            hsv = h
            break
    cr = R()
    u.GetClientRect(hsv, ctypes.byref(cr))
    print('sv id=', sv, 'hsv=', hsv, 'client=', (cr.l, cr.t, cr.r, cr.b))
    cx = 40
    cy = cr.b - 13
    u.SendMessageW(hsv, 0x0201, 1, (cy << 16) | (cx & 0xFFFF))      # WM_LBUTTONDOWN
    u.SendMessageW(hsv, 0x0202, 0, (cy << 16) | (cx & 0xFFFF))      # WM_LBUTTONUP
    p.pump(500)
    print('click 后 index=', w.getViewIndex(sv))
    capture(w.hwnd, os.path.join(out_dir, '_pg_c_back.bmp'))
    capture_screen(w.hwnd, os.path.join(out_dir, '_pg_c_back_s.bmp'))


main()
