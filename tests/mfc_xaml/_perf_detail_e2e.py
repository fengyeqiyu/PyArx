# -*- coding: utf-8 -*-
"""口径 B：真实鼠标点击节点卡片 → 详情卡探点像素变色的端到端墙钟。"""
import os, sys, time, ctypes
import ctypes.wintypes as wt

sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'

import PyMfcXaml as M
import pp_app
import pp_events as E

u = ctypes.windll.user32
u.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
u.ClientToScreen.restype = wt.BOOL
u.GetDC.argtypes = [wt.HWND]; u.GetDC.restype = wt.HDC
u.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
gdi = ctypes.windll.gdi32
gdi.GetPixel.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]; gdi.GetPixel.restype = wt.COLORREF
u.SetForegroundWindow.argtypes = [wt.HWND]
u.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, wt.UINT]
u.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
u.mouse_event.argtypes = [ctypes.c_ulong, ctypes.c_long, ctypes.c_long, ctypes.c_ulong, ctypes.c_void_p]
u.mouse_event.restype = None
u.WindowFromPoint.restype = wt.HWND

MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004

def rgb(color):
    return (color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF)

def click_real():
    for flag in (MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP):
        u.mouse_event(flag, 0, 0, 0, None)

app = pp_app.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(300)
hwnd = app.win.hwnd
u.SetForegroundWindow(hwnd)
u.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
time.sleep(0.15); M.pump(50)

cv = E.find_control(hwnd, pp_app.CV)
panel = E.find_control(hwnd, pp_app.DT_PANEL)
codes = [n['code'] for n in app.nodes[:5]]

# 预热一次，完成 z 序/控件创建；正式计时从隐藏详情开始。
app.fillDetail(codes[0]); M.pump(120)
app.win.savePng(r'G:\PyArx\docs\_detail_e2e_probe.png')

times, pre_post = [], []
for i in range(5):
    code = codes[i % len(codes)]
    app.win.selectPlan(pp_app.CV, '')
    app.hideDetail()
    M.pump(10)
    u.SetForegroundWindow(hwnd)

    prc = wt.RECT(); u.GetWindowRect(panel, ctypes.byref(prc))
    # ShadowBox HWND 外扩了 pad=14；白卡内边距 12，探点取左上内侧空白。
    px, py = prc.left + 40, prc.top + 30
    hdc = u.GetDC(None)
    pre = rgb(gdi.GetPixel(hdc, px, py))

    cr = app.win.cardRectPlan(pp_app.CV, code)
    pt = wt.POINT(int(cr[0] + cr[2] / 2), int(cr[1] + cr[3] / 2))
    u.ClientToScreen(cv, ctypes.byref(pt))
    u.SetCursorPos(pt.x, pt.y)
    time.sleep(0.02)

    t0 = time.perf_counter()
    click_real()

    post = pre
    for _ in range(300):
        M.pump(1)
        c = rgb(gdi.GetPixel(hdc, px, py))
        if c != pre:
            post = c; break
    elapsed = (time.perf_counter() - t0) * 1000
    u.ReleaseDC(None, hdc)

    times.append(elapsed)
    pre_post.append((pre, post))
    print('run%d click=(%d,%d) panel=(%d,%d)-(%d,%d) probe=(%d,%d) pre=%s post=%s -> %.2f ms visible=%s sel=%s' % (
        i + 1, pt.x, pt.y, prc.left, prc.top, prc.right, prc.bottom, px, py, pre, post, elapsed,
        app._detailVisible, app.win.getPlanSelection(pp_app.CV)), flush=True)
    assert app._detailVisible and app.win.getPlanSelection(pp_app.CV) == code, '详情未由真实点击弹出'
    assert post[0] > 230 and post[1] > 230 and post[2] > 230, '探点未落到详情白卡'

def med(x): return sorted(x)[len(x) // 2]
print('口径B 详情真实感知：中位 %.2f ms，均值 %.2f ms（5 次）' % (med(times), sum(times) / len(times)))
print('探点：DT_PANEL 右内侧空白；pre/post RGB 见上')
os._exit(0)
