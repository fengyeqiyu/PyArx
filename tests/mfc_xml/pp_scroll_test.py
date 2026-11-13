# -*- coding: utf-8 -*-
"""侧板滚动验收：真发 WM_MOUSEWHEEL 给宿主窗口，看行区是否跟着滚、数据是否还完整。"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXml as M
import pp_app

DOCS = r'G:\PyArx\docs'
WM_MOUSEWHEEL = 0x020A

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = wt.LPARAM
user32.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
user32.ClientToScreen.restype = wt.BOOL


HWND_TOPMOST = -1
HWND_NOTOPMOST = -2


def front(hwnd, topmost=True):
    """savePng 抓的是屏幕，窗口被别的程序压住就会抓到别人。
    只 SetForegroundWindow 抢不过浏览器/Word，直接临时置顶最可靠。"""
    user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                    ctypes.c_int, ctypes.c_int, wt.UINT]
    user32.SetWindowPos.restype = wt.BOOL
    user32.SetForegroundWindow.argtypes = [wt.HWND]
    user32.SetForegroundWindow(hwnd)
    flag = HWND_TOPMOST if topmost else HWND_NOTOPMOST
    user32.SetWindowPos(hwnd, flag, 60, 40, 0, 0, 0x0001 | 0x0002 | 0x0040)  # NOSIZE|NOMOVE? 用 NOSIZE 保持尺寸
    time.sleep(0.45)


def wheel(hwnd, cx, cy, up):
    pt = wt.POINT(cx, cy)
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    delta = 120 if up else -120
    wparam = (delta & 0xFFFF) << 16
    lparam = (pt.x & 0xFFFF) | ((pt.y & 0xFFFF) << 16)
    user32.SendMessageW(hwnd, WM_MOUSEWHEEL, wparam, lparam)


def main():
    app = pp_app.App()
    app.win.show()
    time.sleep(0.6)
    M.pump(40)

    app.showEditor()
    M.pump(60)
    hwnd = app.win.hwnd
    rx, ry, rw, rh = app._grid_body_rect()
    phrase = '可视 %d 行 / 共 %d 行 / 行高 %d / 起点 %d'
    ok = True

    def check(name, cond, detail):
        nonlocal ok
        print('%s %-26s %s' % ('PASS' if cond else 'FAIL', name, detail))
        if not cond:
            ok = False

    print(phrase % (app._grid_vis, app._grid_rows, app._grid_rh, app._scroll_row))
    check('侧板行高固定', app._grid_rh == pp_app.GRID_RH, '%d px' % app._grid_rh)
    check('需要滚动', app._grid_rows > app._grid_vis,
          '共 %d 行，一屏 %d 行' % (app._grid_rows, app._grid_vis))

    # 往下滚三档
    for _ in range(3):
        wheel(hwnd, rx + 120, ry + 120, up=False)
    M.pump(30)
    check('滚轮向下', app._scroll_row > 0, '起点 0 -> %d' % app._scroll_row)
    front(hwnd)
    app.win.savePng(os.path.join(DOCS, '_pp_scroll_mid.png'))
    print('  截图 _pp_scroll_mid.png', os.path.getsize(os.path.join(DOCS, '_pp_scroll_mid.png')), 'B')

    # 滚到底
    for _ in range(20):
        wheel(hwnd, rx + 120, ry + 120, up=False)
    M.pump(30)
    bottom = max(0, app._grid_rows - app._grid_vis)
    check('到底夹住', app._scroll_row == bottom, '起点 %d == %d' % (app._scroll_row, bottom))
    front(hwnd)
    app.win.savePng(os.path.join(DOCS, '_pp_scroll_end.png'))
    print('  截图 _pp_scroll_end.png', os.path.getsize(os.path.join(DOCS, '_pp_scroll_end.png')), 'B')

    # 往上滚回顶
    for _ in range(20):
        wheel(hwnd, rx + 120, ry + 120, up=True)
    M.pump(30)
    check('滚回顶部', app._scroll_row == 0, '起点 %d' % app._scroll_row)

    # 滚动后数据仍然完整（隐藏行也要能读回来）
    nodes, err = app.collectGrid()
    check('滚动后数据完整', nodes is not None and len(nodes) == app._grid_rows,
          '%s 条 / 期望 %d 条 %s' % (len(nodes) if nodes else 'None', app._grid_rows, err))

    # 滚轮只在侧板行区内有效：画布区域滚轮不应该动侧板
    app.win.setPlanOptions(pp_app.CV, True, False, -1)
    z0 = app.win.getPlanZoom(pp_app.CV)
    for _ in range(3):
        wheel(hwnd, pp_app.CV_X + 400, pp_app.CV_Y + 300, up=False)
    M.pump(30)
    check('画布滚轮不串到侧板', app._scroll_row == 0, '起点 %d' % app._scroll_row)

    app.hideEditor()
    M.pump(30)
    for _ in range(3):
        wheel(hwnd, rx + 120, ry + 120, up=False)
    M.pump(30)
    check('关侧板后不吃滚轮', app._scroll_row == 0, '起点 %d' % app._scroll_row)

    print('\n汇总：%s' % ('PASS' if ok else 'FAIL'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
