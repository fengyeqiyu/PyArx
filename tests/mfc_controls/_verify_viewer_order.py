# -*- coding: utf-8 -*-
"""_verify_viewer_order.py — 预览器（Viewer.exe）里 Level / TabIndex 是否生效。

跑法（独立进程，不碰 PyArxCore / PythonEngine）：
    $env:PYTHONPATH 不用于本脚本；直接 python.exe _verify_viewer_order.py

覆盖：
  1. 预览器能把 Level="1" 的 B 块压在 Level="0" 的 A 块上面（真屏幕像素采样）
  2. Tab 从 e1(TabIndex=3) 出发走 btnTop(4) → btnBottom(5) → btnSwap(6)
  3. Shift+Tab 从 e1 回到 e3(TabIndex=2)
判据：末行打印 VIEWER ORDER TEST PASSED
"""
import ctypes
import ctypes.wintypes as wt
import os
import struct
import sys
import time

u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.SetProcessDPIAware()

u.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.GetWindowRect.restype = wt.BOOL
u.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.ClientToScreen.argtypes = [wt.HWND, ctypes.POINTER(wt.POINT)]
u.GetDlgCtrlID.argtypes = [wt.HWND]
u.GetDlgCtrlID.restype = ctypes.c_int
u.GetParent.argtypes = [wt.HWND]
u.GetParent.restype = wt.HWND
u.GetWindow.argtypes = [wt.HWND, ctypes.c_uint]
u.GetWindow.restype = wt.HWND
u.EnumChildWindows.argtypes = [wt.HWND, ctypes.c_void_p, wt.LPARAM]
u.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
u.SetForegroundWindow.argtypes = [wt.HWND]
u.SetFocus.argtypes = [wt.HWND]
u.GetGUIThreadInfo.argtypes = [wt.DWORD, ctypes.c_void_p]


class GUITHREADINFO(ctypes.Structure):
    _fields_ = [('cbSize', wt.DWORD), ('flags', wt.DWORD),
                ('hwndActive', wt.HWND), ('hwndFocus', wt.HWND),
                ('hwndCapture', wt.HWND), ('hwndMenuOwner', wt.HWND),
                ('hwndMoveSize', wt.HWND), ('hwndCaret', wt.HWND),
                ('rcCaret', wt.RECT)]


VIEWER = r'G:\PyArx\X64\Release\Viewer.exe'
XAML = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'level_tab_viewer.xaml')
OUT = r'G:\PyArx\docs'
GW_CHILD, GW_HWNDNEXT = 5, 2

failures = []


def check(name, ok, detail=''):
    print(('PASS ' if ok else 'FAIL ') + name + ('  ' + detail if detail else ''))
    if not ok:
        failures.append(name)


def find_window(title):
    found = []
    u.EnumWindows.argtypes = [ctypes.c_void_p, wt.LPARAM]
    u.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        buf = ctypes.create_unicode_buffer(512)
        u.GetWindowTextW(h, buf, 512)
        if buf.value == title or title in buf.value:
            found.append(h)
            return 0
        return 1

    u.EnumWindows(ctypes.cast(cb, ctypes.c_void_p), 0)
    return found[0] if found else None


def children(h):
    out = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(w, _):
        out.append(w)
        return 1

    u.EnumChildWindows(h, cb, 0)
    return out


def find_by_id(top, cid):
    for h in children(top):
        if u.GetDlgCtrlID(h) == cid:
            return h
    return None


def rect_of(h):
    r = wt.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    return (r.left, r.top, r.right, r.bottom)


def grab(x, y, w, h):
    hdc = u.GetDC(0)
    mdc = g.CreateCompatibleDC(hdc)
    bmp = g.CreateCompatibleBitmap(hdc, w, h)
    g.SelectObject(mdc, bmp)
    g.BitBlt(mdc, 0, 0, w, h, hdc, x, y, 0x00CC0020)
    bits = ctypes.create_string_buffer(w * h * 4)
    class BI(ctypes.Structure):
        _fields_ = [('biSize', wt.DWORD), ('biWidth', ctypes.c_long),
                    ('biHeight', ctypes.c_long), ('biPlanes', wt.WORD),
                    ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                    ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', ctypes.c_long),
                    ('biYPelsPerMeter', ctypes.c_long), ('biClrUsed', wt.DWORD),
                    ('biClrImportant', wt.DWORD)]
    bi = BI(ctypes.sizeof(BI), w, -h, 1, 32, 0, 0, 0, 0, 0, 0)
    g.GetDIBits(mdc, bmp, 0, h, bits, ctypes.byref(bi), 0)
    g.DeleteObject(bmp)
    g.DeleteDC(mdc)
    u.ReleaseDC(0, hdc)
    return bits.raw, w


def px(data, w, x, y):
    i = (y * w + x) * 4
    return (data[i + 2], data[i + 1], data[i])


def dist(a, b):
    return sum((a[i] - b[i]) ** 2 for i in range(3))


A_RGB = (192, 57, 43)     # #C0392B
B_RGB = (46, 134, 193)    # #2E86C1


def sample_overlap(top):
    ha = find_by_id(top, 1000)
    hb = find_by_id(top, 1001)
    ra, rb = rect_of(ha), rect_of(hb)
    ix1, iy1 = max(ra[0], rb[0]), max(ra[1], rb[1])
    ix2, iy2 = min(ra[2], rb[2]), min(ra[3], rb[3])
    if ix2 <= ix1 or iy2 <= iy1:
        return None
    # 采右下角内侧 9×9（避开中间的文字），按多数投票判定是哪一块
    x0, y0 = ix2 - 12, iy2 - 12
    data, w = grab(x0, y0, 9, 9)
    votes = {'A': 0, 'B': 0, 'other': 0}
    for yy in range(9):
        for xx in range(9):
            c = px(data, w, xx, yy)
            da, db = dist(c, A_RGB), dist(c, B_RGB)
            if min(da, db) > 4000:
                votes['other'] += 1
            elif da < db:
                votes['A'] += 1
            else:
                votes['B'] += 1
    return votes


def focus_id_of(pid_thread):
    info = GUITHREADINFO()
    info.cbSize = ctypes.sizeof(GUITHREADINFO)
    u.GetGUIThreadInfo(pid_thread, ctypes.byref(info))
    return u.GetDlgCtrlID(info.hwndFocus), info.hwndFocus


def key(vk, shift=False):
    if shift:
        u.keybd_event(0x10, 0, 0, 0)
    u.keybd_event(vk, 0, 0, 0)
    time.sleep(0.05)
    u.keybd_event(vk, 0, 2, 0)
    if shift:
        u.keybd_event(0x10, 0, 2, 0)
    time.sleep(0.15)


def click(h):
    x1, y1, x2, y2 = rect_of(h)
    u.SetCursorPos((x1 + x2) // 2, (y1 + y2) // 2)
    time.sleep(0.05)
    u.mouse_event(0x0002, 0, 0, 0, 0)
    u.mouse_event(0x0004, 0, 0, 0, 0)
    time.sleep(0.2)


def main():
    import subprocess
    if not os.path.exists(VIEWER):
        print('缺少 ' + VIEWER)
        return 1
    proc = subprocess.Popen([VIEWER, XAML], cwd=os.path.dirname(VIEWER))
    top = None
    for _ in range(20):
        time.sleep(0.5)
        top = find_window('预览器 · 层次与 Tab 序号')
        if top is not None:
            break
    if top is None:
        print('未找到预览器窗口')
        proc.terminate()
        return 1

    u.SetForegroundWindow(top)
    u.ShowWindow(top, 5)
    time.sleep(0.8)

    # ---- 1. Level：B(Level=1) 应该压在 A(Level=0) 上 ----
    votes = sample_overlap(top)
    if votes is None:
        check('重叠区取样', False, '两块没有重叠')
    else:
        check('Level：B 压在 A 上（重叠区像素）', votes['B'] > 0 and votes['A'] == 0, str(votes))
    # 反过来验一遍：两块不重叠区各取一点，确认颜色都在
    ha, hb = find_by_id(top, 1000), find_by_id(top, 1001)
    print('  A rect=%s  B rect=%s' % (rect_of(ha), rect_of(hb)))

    # ---- 2. Tab 序：e1(3) → btnTop(4) → btnBottom(5) → btnSwap(6) ----
    e1 = find_by_id(top, 1002)
    u.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]
    u.GetWindowThreadProcessId.restype = wt.DWORD
    tid = u.GetWindowThreadProcessId(top, None)
    fid = 0
    for _ in range(4):
        u.SetForegroundWindow(top)
        time.sleep(0.2)
        click(e1)
        time.sleep(0.3)
        fid, _ = focus_id_of(tid)
        if fid == 1002:
            break
    check('点中 e1 拿到焦点', fid == 1002, 'focus id=%d' % fid)

    seq = []
    for _ in range(3):
        key(0x09)
        fid, _ = focus_id_of(tid)
        seq.append(fid)
    check('Tab 序 e1→btnTop→btnBottom→btnSwap', seq == [1005, 1006, 1007], str(seq))

    # ---- 3. Shift+Tab：从 e1 反走回 e3(2) ----
    click(e1)
    time.sleep(0.2)
    key(0x09, shift=True)
    fid, _ = focus_id_of(tid)
    check('Shift+Tab：e1→e3', fid == 1004, 'focus id=%d' % fid)

    # ---- 4. 属性确实落到了窗口上（Seq / TabIndex）----
    props = {}
    for cid, name in ((1002, 'e1'), (1005, 'btnTop')):
        h = find_by_id(top, cid)
        if h:
            props[name] = (u.GetPropW(h, 'PyMfcSeq'), u.GetPropW(h, 'PyMfcTabIndex'))
    check('窗口属性 PyMfcSeq/TabIndex 已写入', all(v[0] and v[1] for v in props.values()), str(props))

    proc.terminate()
    print('')
    if failures:
        print('VIEWER ORDER TEST FAILED: ' + ', '.join(failures))
        return 1
    print('VIEWER ORDER TEST PASSED')
    return 0


if __name__ == '__main__':
    sys.exit(main())
