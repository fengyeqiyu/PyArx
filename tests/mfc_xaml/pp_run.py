# -*- coding: utf-8 -*-
"""常驻运行版：打开窗口并保持不退出，供手动体验（关闭窗口即退出）。"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import PyMfcXaml as M
import pp_app

user32 = ctypes.WinDLL('user32', use_last_error=True)
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                ctypes.c_int, ctypes.c_int, wt.UINT]
user32.IsWindow.argtypes = [wt.HWND]
user32.IsWindow.restype = wt.BOOL

app = pp_app.App()
app.win.show()
M.pump(80)

# 直接进入编辑态，侧板一屏 30 行、共 39 行，滚轮即可看到换行
# 直接进入编辑态，侧板一屏 30 行、共 39 行，滚轮即可看到换行
if '--noedit' not in sys.argv:
    app.showEditor()
    M.pump(150)

if '--dual' in sys.argv:
    app.onDualSource()
    M.pump(150)


user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
user32.IsZoomed.argtypes = [wt.HWND]
user32.IsZoomed.restype = wt.BOOL

DOCK_W = 1440          # 副屏右侧停靠宽度：整屏会盖住聊天窗口，太窄又看不清图


def secondScreenRect():
    """副屏工作区里给本窗口预留的矩形 (x, y, w, h)；没有副屏返回 None。"""
    rects = []

    class MI(ctypes.Structure):
        _fields_ = [('cbSize', ctypes.c_ulong), ('rcMonitor', wt.RECT),
                    ('rcWork', wt.RECT), ('dwFlags', ctypes.c_ulong)]

    PROC = ctypes.WINFUNCTYPE(ctypes.c_int, wt.HMONITOR, wt.HDC,
                              ctypes.POINTER(wt.RECT), wt.LPARAM)

    def cb(hmon, hdc, lprc, lparam):
        mi = MI()
        mi.cbSize = ctypes.sizeof(MI)
        if user32.GetMonitorInfoW(hmon, ctypes.byref(mi)):
            rects.append((mi.rcWork.left, mi.rcWork.top,
                          mi.rcWork.right - mi.rcWork.left,
                          mi.rcWork.bottom - mi.rcWork.top, mi.dwFlags))
        return 1

    user32.EnumDisplayMonitors(None, None, PROC(cb), 0)
    if len(rects) < 2:
        return None
    sec = next((r for r in rects if not (r[4] & 1)), None)   # 1 = PRIMARY
    if sec is None:
        return None
    x, y, w, h = sec[0], sec[1], sec[2], sec[3]
    dockW = min(DOCK_W, w)
    return (x + w - dockW, y, dockW, h)


def placeOnSecondScreen(hwnd):
    """摆到副屏并逐轮校验它真的停在那儿。
    Win11 会在窗口显示/激活后把位置"还原"成它记住的状态（常见是主屏最大化），
    所以这里必须 摆→等→量→不对再摆 地闭环，不能只调一次 SetWindowPos。"""
    pos = secondScreenRect()
    if pos is None:
        return None
    x, y, w, h = pos
    ok = 0
    for _ in range(10):
        user32.SetWindowPos(hwnd, -1, x, y, w, h, 0x0050)   # TOPMOST|NOACTIVATE|SHOWWINDOW
        M.pump(400)
        r = wt.RECT()
        user32.GetWindowRect(hwnd, ctypes.byref(r))
        if (not user32.IsZoomed(hwnd) and r.left == x and r.top == y
                and r.right - r.left == w and r.bottom - r.top == h):
            ok += 1
            if ok >= 3:          # 连续三轮都对，才算稳定
                return pos
        else:
            ok = 0
            if user32.IsZoomed(hwnd):
                user32.ShowWindow(hwnd, 9)    # 被还原成最大化就先缩回普通层
                M.pump(300)
    return pos



_pos = placeOnSecondScreen(app.win.hwnd)
print('窗口已放到副屏：%s' % (_pos,), flush=True)
M.pump(250)
if _pos:
    for _i in range(2):
        M.pump(400)
        app.rebuild(fit=True)      # 换屏后按新画布尺寸重排 + 适配
        M.pump(120)
    print('画布 %sx%s 视图 %s' % (app.cvW, app.cvH, app.win.getPlanView(pp_app.CV)), flush=True)
# 尺寸变过之后按新画布重算 fit（否则 C++ 那套按宽算的比例会把下方裁掉）
app.fitView()
M.pump(150)

hwnd = app.win.hwnd
# 只 SetForegroundWindow；这里再 SetWindowPos(None,...) 会把 TOPMOST 掉回普通层，
# 副屏上的聊天窗口就会盖住它（这正是"看不到在副屏"的原因）。
user32.SetForegroundWindow(hwnd)
M.pump(300)
# 收尾再摆一次：窗口显示 + 激活后 Win11 有时会把窗口挪回它记住的位置/主屏，
# 启动链末尾补一次 SetWindowPos(TOPMOST) 才真的停在副屏。
if _pos:
    _pos2 = placeOnSecondScreen(hwnd)
    print('副屏收尾位置：%s' % (_pos2,), flush=True)
    M.pump(200)
print('窗口已打开：表格共 %d 行（新自绘 PyMfcTable），关闭窗口即退出。'
      % app._grid_rows, flush=True)

if '--shot' in sys.argv:
    time.sleep(0.3)
    user32.SetWindowPos(hwnd, wt.HWND(-1), 40, 30, 0, 0, 0x0001 | 0x0040)
    M.pump(400)
    app.win.savePng(r'G:\PyArx\docs\_pp_live.png')
    M.pump(80)
    print('已截图 docs/_pp_live.png', flush=True)

# 常驻消息循环：只有这一步。窗口位置由启动链里的 placeOnSecondScreen 一次性摆好，
# 之后不再周期性地重摆（不再有看门狗）。
while user32.IsWindow(hwnd):
    M.pump(50)
    app.checkHotkeys()
    if getattr(app, '_pending_rebuild', False):
        app.rebuild(fit=True)
    if getattr(app, '_pendingMini', None):
        txt = app._pendingMini
        app._pendingMini = None
        if txt is True:
            # 直接拷主画布已解析好的场景（省掉再解析一遍 4.8 MB 的 40~60 ms）
            app.win.copyPowerScene(pp_app.MINI, pp_app.CV)
        else:
            app.win.loadPowerScene(pp_app.MINI, txt)
        app.win.setPlanOptions(pp_app.MINI, False, True, pp_app.CV)
print('窗口已关闭，进程退出。', flush=True)
