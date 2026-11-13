# -*- coding: utf-8 -*-
"""_verify_designer_order.py — 设计器里 Level 是否决定"谁压在上面"（用命中测试取证）。

做法：
  文档里 A 声明在前但 Level=1，B 声明在后但 Level=0，两块半重叠。
  在重叠区点一下：命中的必须是最上面那块（A）。
  判定依据：设计器标题栏里的 "Name: a | Class: TextBlock"。
  状态栏是 D2D 自绘、Win32 读不到文字，所以 updateStatus() 把同一份结论同步到标题栏，
  这里只读标题（GetWindowTextW），不依赖 OCR，也不依赖像素。

覆盖三点：
  1) 重叠区命中 Level 更高的 A；
  2) B 独有区域命中 B；
  3) A 独有区域命中 A —— 证明不是"永远选第一个"，命中跟随坐标。

输出：_ds_doc.bmp / _ds_hit.bmp（取证图），控制台 PASS/FAIL 与总计。
"""
import ctypes
import ctypes.wintypes as wt
import os
import struct
import subprocess
import sys
import time

u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.SetProcessDPIAware()
u.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
u.GetWindowRect.restype = wt.BOOL
u.EnumWindows.argtypes = [ctypes.c_void_p, wt.LPARAM]
u.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]

DESIGNER = r'G:\PyArx\X64\Release\UIDesign.exe'
DOC = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'level_tab_design.xaml')
OUT = r'G:\PyArx\docs'

# 画布在窗口客户区的左上角（100% 缩放），由 _ds_doc.bmp 量出
CANVAS_X, CANVAS_Y = 280, 185

PASS, FAIL = [], []


def check(name, ok, detail=''):
    (PASS if ok else FAIL).append(name + ('  ' + detail if detail else ''))
    print(('  OK   ' if ok else '  FAIL ') + name + (('  | ' + detail) if detail else ''), flush=True)


def find(title):
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        b = ctypes.create_unicode_buffer(1024)
        u.GetWindowTextW(h, b, 1024)
        if title in b.value:
            found.append(h)
            return 0
        return 1

    u.EnumWindows(ctypes.cast(cb, ctypes.c_void_p), 0)
    return found[0] if found else None


def title_of(h):
    b = ctypes.create_unicode_buffer(1024)
    u.GetWindowTextW(h, b, 1024)
    return b.value


class BIH(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', ctypes.c_long),
                ('biHeight', ctypes.c_long), ('biPlanes', wt.WORD),
                ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', ctypes.c_long),
                ('biYPelsPerMeter', ctypes.c_long), ('biClrUsed', wt.DWORD),
                ('biClrImportant', wt.DWORD)]


def grab(x, y, w, h, bmp_path):
    hdc = u.GetDC(0)
    mdc = g.CreateCompatibleDC(hdc)
    bmp = g.CreateCompatibleBitmap(hdc, w, h)
    g.SelectObject(mdc, bmp)
    g.BitBlt(mdc, 0, 0, w, h, hdc, x, y, 0x00CC0020)
    bi = BIH()
    bi.biSize = ctypes.sizeof(BIH)
    bi.biWidth, bi.biHeight, bi.biPlanes, bi.biBitCount = w, -h, 1, 24
    size = ((w * 3 + 3) // 4) * 4 * h
    bits = ctypes.create_string_buffer(size)
    g.GetDIBits(mdc, bmp, 0, h, bits, ctypes.byref(bi), 0)
    fh = struct.pack('<IHH', 14 + 40 + size, 0, 0) + struct.pack('<I', 14 + 40)
    ih = struct.pack('<IiiHHIIiiII', 40, w, -h, 1, 24, 0, size, 0, 0, 0, 0)
    with open(bmp_path, 'wb') as f:
        f.write(b'BM' + fh + ih + bits.raw)
    g.DeleteObject(bmp)
    g.DeleteDC(mdc)
    u.ReleaseDC(0, hdc)


def click_doc(r, docx, docy):
    cx, cy = r.left + CANVAS_X + docx, r.top + CANVAS_Y + docy
    u.SetCursorPos(cx, cy)
    time.sleep(0.15)
    u.mouse_event(0x0002, 0, 0, 0, 0)   # 左键按下
    u.mouse_event(0x0004, 0, 0, 0, 0)   # 左键抬起
    time.sleep(0.45)
    return cx, cy


def ensure_foreground(h, tries=15, gap=0.15):
    """批跑时前一个脚本可能还捏着前台锁，SetForegroundWindow 会被拒绝；
    必须读回 GetForegroundWindow 确认真的拿到前台，否则点击会落到别的窗口上。"""
    u.GetForegroundWindow.restype = ctypes.c_void_p
    for _ in range(tries):
        u.SetForegroundWindow(h)
        time.sleep(gap)
        if u.GetForegroundWindow() == h:
            return True
    return False


def click_for(h, r, docx, docy, want, tries=3):
    """点一下并读回标题；没达到预期就重提前台再点，避免一次失手就判失败。"""
    for _ in range(tries):
        ensure_foreground(h)
        click_doc(r, docx, docy)
        t = title_of(h)
        if want in t:
            return t
    return title_of(h)


def main():
    p = subprocess.Popen([DESIGNER, DOC], cwd=os.path.dirname(DESIGNER))
    h = None
    for _ in range(24):
        time.sleep(0.5)
        h = find('PyMfc UI Designer')
        if h:
            break
    if not h:
        print('未找到设计器窗口', flush=True)
        p.terminate()
        return 1

    ensure_foreground(h)
    time.sleep(1.2)
    r = wt.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    print('designer rect', (r.left, r.top, r.right, r.bottom), flush=True)
    grab(r.left, r.top, r.right - r.left, r.bottom - r.top,
         os.path.join(OUT, '_ds_doc.bmp'))

    # 初始态：设计器默认选中文档根 <Window>，不应该已经选中 a/b
    t0 = title_of(h)
    check('初始态默认选中根 Window', 'Class: Window' in t0 and 'Name: a' not in t0, t0[-46:])

    # 1) 重叠区：A(30..200,30..120) 与 B(70..240,60..150) 的交叠点 (120,95)
    t = click_for(h, r, 120, 95, 'Name: a')
    check('重叠区命中 Level 更高的 A', 'Name: a' in t and 'TextBlock' in t, t[-46:])
    check('重叠区没有误选 B', 'Name: b' not in t, t[-46:])
    grab(r.left, r.top, r.right - r.left, r.bottom - r.top,
         os.path.join(OUT, '_ds_hit.bmp'))

    # 2) B 独有区 (220,140)：只被 B 覆盖
    t = click_for(h, r, 220, 140, 'Name: b')
    check('B 独有区命中 B', 'Name: b' in t, t[-46:])

    # 3) A 独有区 (50,40)：只被 A 覆盖
    t = click_for(h, r, 50, 40, 'Name: a')
    check('A 独有区命中 A', 'Name: a' in t, t[-46:])

    print('', flush=True)
    print('设计器命中验收：通过 %d，失败 %d' % (len(PASS), len(FAIL)), flush=True)
    for f in FAIL:
        print('  失败：' + f, flush=True)
    print('DESIGNER ORDER TEST ' + ('PASSED' if not FAIL else 'FAILED'), flush=True)
    print('saved _ds_doc.bmp / _ds_hit.bmp', flush=True)

    p.terminate()
    return 0 if not FAIL else 1


if __name__ == '__main__':
    sys.exit(main())
