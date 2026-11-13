# -*- coding: utf-8 -*-
"""_verify_level_tab.py — 控件通用属性 Level / TabIndex 验收（纯 Python，独立进程）。

验 12 条：
 1) XAML 上的 Level / TabIndex 被读进控件（属性面板同源）；
 2) Level 大的一方在 z 序上更靠上；
 3) 重叠区的像素确实是"上面那块"的颜色（真的画上去了，不只是内部状态）；
 4) setLevel 改层次 → z 序与像素都跟着变；
 5) bringToFront 把目标顶到最上；
 6) sendToBack 把目标压到最下；
 7) Tab 顺序由 TabIndex 决定，而不是声明顺序 / z 序；
 8) Tab 会绕回第一个；
 9) Shift+Tab 反向走；
 10) 改 Level 不影响 Tab 顺序（两者解耦）；
 11) getLevel / getTabIndex 回读与设置一致；
 12) Tab 只走可见且可用的控件（隐藏的不进队列）。
"""
import ctypes
import os
import struct
from ctypes import wintypes

import PyMfcXaml as p
from demo_level_tab import build
from _verify_flickable import capture

u = ctypes.windll.user32
u.PostMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.MapWindowPoints.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
u.GetDlgCtrlID.restype = ctypes.c_int
u.SetFocus.argtypes = [ctypes.c_void_p]
u.SetFocus.restype = ctypes.c_void_p
u.GetFocus.restype = ctypes.c_void_p
u.SetFocus.argtypes = [ctypes.c_void_p]
u.IsWindowVisible.argtypes = [ctypes.c_void_p]
u.ShowWindow.argtypes = [ctypes.c_void_p, ctypes.c_int]
u.keybd_event.argtypes = [ctypes.c_byte, ctypes.c_byte, wintypes.DWORD, ctypes.c_void_p]
u.GetWindow.argtypes = [ctypes.c_void_p, ctypes.c_uint]
u.GetWindow.restype = ctypes.c_void_p

CB = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)
WM_KEYDOWN = 0x0100
VK_TAB, VK_SHIFT = 0x09, 0x10
KEYEVENTF_KEYUP = 0x0002
GW_CHILD, GW_HWNDNEXT = 5, 2

A_RGB = (192, 57, 43)      # #C0392B
B_RGB = (46, 134, 193)     # #2E86C1

RESULTS = []


def check(name, ok, detail=''):
    RESULTS.append((name, bool(ok), detail))
    print(('  OK  ' if ok else ' FAIL ') + name + (('  |  ' + detail) if detail else ''))


class R(ctypes.Structure):
    _fields_ = [('l', wintypes.LONG), ('t', wintypes.LONG),
                ('r', wintypes.LONG), ('b', wintypes.LONG)]


class BMIH(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', wintypes.LONG),
                ('biHeight', wintypes.LONG), ('biPlanes', wintypes.WORD),
                ('biBitCount', wintypes.WORD), ('biCompression', wintypes.DWORD),
                ('biSizeImage', wintypes.DWORD), ('biXPelsPerMeter', wintypes.LONG),
                ('biYPelsPerMeter', wintypes.LONG), ('biClrUsed', wintypes.DWORD),
                ('biClrImportant', wintypes.DWORD)]


g = ctypes.windll.gdi32
g.CreateCompatibleDC.argtypes = [ctypes.c_void_p]
g.CreateCompatibleDC.restype = ctypes.c_void_p
g.CreateDIBSection.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD,
                               ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD]
g.CreateDIBSection.restype = ctypes.c_void_p
g.SelectObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.SelectObject.restype = ctypes.c_void_p
g.BitBlt.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                     ctypes.c_void_p, ctypes.c_int, ctypes.c_int, wintypes.DWORD]
u.GetDC.argtypes = [ctypes.c_void_p]
u.GetDC.restype = ctypes.c_void_p
# 不声明 GetWindowRect 的签名，64 位句柄会被截成 32 位，拿回来是一堆 0
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.POINTER(R)]
u.GetWindowRect.restype = ctypes.c_int
u.SetForegroundWindow.argtypes = [ctypes.c_void_p]
u.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                           ctypes.c_int, ctypes.c_int, ctypes.c_uint]


def screen_shot(x, y, w, h, out):
    """真屏幕抓一块区域存成 32bpp 顶朝下 BMP。
    自绘控件（D2D）用 PrintWindow 抓出来是全黑，只有真屏幕才有像素。"""
    bi = BMIH()
    bi.biSize = ctypes.sizeof(BMIH)
    bi.biWidth, bi.biHeight, bi.biPlanes, bi.biBitCount = w, -h, 1, 32
    bits = ctypes.c_void_p()
    bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
    mdc = g.CreateCompatibleDC(0)
    g.SelectObject(mdc, bmp)
    g.BitBlt(mdc, 0, 0, w, h, u.GetDC(None), x, y, 0x00CC0020)   # SRCCOPY
    data = ctypes.string_at(bits, w * h * 4)
    header = struct.pack('<2sIHHI', b'BM', 14 + 40 + len(data), 0, 0, 54)
    dib = struct.pack('<IiiHHIIiiII', 40, w, -h, 1, 32, 0, len(data), 0, 0, 0, 0)
    with open(out, 'wb') as f:
        f.write(header + dib + data)
    return out


def find_by_id(w, cid):
    got = []

    def cb(h, _):
        if u.GetDlgCtrlID(ctypes.c_void_p(h)) == cid:
            got.append(h)
            return 0
        return 1

    u.EnumChildWindows(ctypes.c_void_p(w.hwnd), CB(cb), 0)
    return got[0] if got else None


def zorder(w):
    """控件层的 z 序：靠前者在上（GW_HWNDFIRST = 最上面）。
    顶层窗 → 一个 id=0 的宿主窗 → 各控件。"""
    top = ctypes.c_void_p(w.hwnd)
    inner = u.GetWindow(top, GW_CHILD)
    out, h = [], u.GetWindow(ctypes.c_void_p(inner), GW_CHILD)
    while h:
        out.append(u.GetDlgCtrlID(ctypes.c_void_p(h)))
        h = u.GetWindow(ctypes.c_void_p(h), GW_HWNDNEXT)
    return out


def read_px(bmp, x, y):
    d = open(bmp, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w = struct.unpack_from('<i', d, 18)[0]
    h = struct.unpack_from('<i', d, 22)[0]
    bpp = struct.unpack_from('<H', d, 28)[0]
    stride = ((w * bpp // 8 + 3) // 4) * 4
    hh = abs(h)
    row = y if h < 0 else (hh - 1 - y)
    i = off + row * stride + x * 4
    return (d[i + 2], d[i + 1], d[i])


def near(c, want, tol=18):
    return c is not None and all(abs(c[k] - want[k]) <= tol for k in range(3))


def shot(w, path):
    capture(ctypes.c_void_p(w.hwnd), path)
    return path


def sample_overlap(w, ha, hb, path):
    """采 A、B 重叠区里"离线、离字"的一小块，按多数像素判谁在上面。
    走真屏幕：自绘控件用 PrintWindow 抓出来是全黑，只有屏幕像素才看得出层次。
    采样点避开中心——那里正好压着 B 的文字，抗锯齿后是混色，会被误判。"""
    ra, rb = R(), R()
    u.GetWindowRect(ctypes.c_void_p(ha), ctypes.byref(ra))
    u.GetWindowRect(ctypes.c_void_p(hb), ctypes.byref(rb))
    l = max(ra.l, rb.l)
    t = max(ra.t, rb.t)
    r = min(ra.r, rb.r)
    btm = min(ra.b, rb.b)
    # 取右下角内侧一小块：远离文字，一定落在两块的交集里
    px = r - 12
    py = btm - 12
    screen_shot(px - 4, py - 4, 9, 9, path)
    d = open(path, 'rb').read()
    votes = {'A': 0, 'B': 0}
    for iy in range(9):
        for ix in range(9):
            c = read_px(path, ix, iy)
            da = sum(abs(c[k] - A_RGB[k]) for k in range(3))
            db = sum(abs(c[k] - B_RGB[k]) for k in range(3))
            if da < db:
                votes['A'] += 1
            elif db < da:
                votes['B'] += 1
    return 'A' if votes['A'] >= votes['B'] else 'B', votes


def focus_of():
    h = u.GetFocus()
    return h, u.GetDlgCtrlID(ctypes.c_void_p(h)) if h else 0


def tab(h):
    u.PostMessageW(ctypes.c_void_p(h), WM_KEYDOWN, ctypes.c_void_p(VK_TAB), ctypes.c_void_p(0))
    p.pump(60)


def shift_tab(h):
    u.keybd_event(ctypes.c_byte(VK_SHIFT), ctypes.c_byte(0), 0, None)
    u.PostMessageW(ctypes.c_void_p(h), WM_KEYDOWN, ctypes.c_void_p(VK_TAB), ctypes.c_void_p(0))
    p.pump(60)
    u.keybd_event(ctypes.c_byte(VK_SHIFT), ctypes.c_byte(0), KEYEVENTF_KEYUP, None)


def main():
    names = {}
    w = p.guiload(build(), names=names)
    w.show()
    # 真屏幕采样需要窗口在最前面：自绘控件只有屏幕像素才抓得到
    u.SetWindowPos(ctypes.c_void_p(w.hwnd), ctypes.c_void_p(-1), 60, 60, 0, 0, 0x0001 | 0x0040)
    u.SetForegroundWindow(ctypes.c_void_p(w.hwnd))
    p.pump(500)
    a, b = names['a'], names['b']
    e1, e2, e3 = names['e1'], names['e2'], names['e3']
    tmp = os.path.join(os.path.dirname(os.path.abspath(__file__)), '_lt_shot.bmp')
    ha, hb = find_by_id(w, a), find_by_id(w, b)

    # 1) 属性读回
    check('1 XAML Level/TabIndex 读入',
          w.getLevel(a) == 0 and w.getLevel(b) == 1 and
          w.getTabIndex(e1) == 3 and w.getTabIndex(e2) == 1 and w.getTabIndex(e3) == 2,
          'A=%d B=%d tab(e1,e2,e3)=%d,%d,%d' % (w.getLevel(a), w.getLevel(b),
                                                w.getTabIndex(e1), w.getTabIndex(e2), w.getTabIndex(e3)))

    # 2) z 序：Level 1 的 B 在 A 之上
    z = zorder(w)
    check('2 Level 大的在 z 序更靠上', z.index(b) < z.index(a), 'z=%s' % z)

    # 3) 像素：重叠区是 B 的颜色（B 的 Level=1 更大）
    c0, v0 = sample_overlap(w, ha, hb, tmp)
    check('3 重叠区像素 = 上面那块(B)', c0 == 'B', '覆盖=%s 票数=%s 期望=B' % (c0, v0))

    # 4) setLevel 改层次
    w.setLevel(a, 2)
    p.pump(120)
    z2 = zorder(w)
    c1, v1 = sample_overlap(w, ha, hb, tmp)
    check('4 setLevel 后 A 压住 B（z 序 + 像素）',
          z2.index(a) < z2.index(b) and c1 == 'A',
          'z=%s 覆盖=%s 票数=%s 期望=A' % (z2, c1, v1))

    # 5) bringToFront
    w.bringToFront(b)
    p.pump(120)
    z3 = zorder(w)
    c2, v2 = sample_overlap(w, ha, hb, tmp)
    check('5 bringToFront 把 B 顶到最上',
          z3.index(b) < z3.index(a) and c2 == 'B',
          'B.Level=%d z=%s 覆盖=%s 票数=%s' % (w.getLevel(b), z3, c2, v2))

    # 6) sendToBack
    w.sendToBack(b)
    p.pump(120)
    z4 = zorder(w)
    c3, v3 = sample_overlap(w, ha, hb, tmp)
    check('6 sendToBack 把 B 压到最下',
          z4.index(a) < z4.index(b) and c3 == 'A',
          'B.Level=%d z=%s 覆盖=%s 票数=%s' % (w.getLevel(b), z4, c3, v3))

    # 7) Tab 顺序由 TabIndex 决定（声明顺序是 e1,e2,e3，但序号是 e2(1),e3(2),e1(3)，
    #    后面接着按钮 4/5/6）：e1 → btnTop → btnBottom → btnSwap → 绕回 e2
    bTop, bBottom, bSwap = names['btnTop'], names['btnBottom'], names['btnSwap']
    u.SetFocus(ctypes.c_void_p(find_by_id(w, e1)))
    p.pump(60)
    seq = []
    for _ in range(4):
        cur, _cid = focus_of()
        tab(cur)
        seq.append(focus_of()[1])
    check('7/8 Tab 按 TabIndex 走并且绕回',
          seq == [bTop, bBottom, bSwap, e2],
          'e1 -> %s' % ' -> '.join(str(x) for x in seq))

    # 9) Shift+Tab 反向：e1(3) 的反向是 e3(2)
    u.SetFocus(ctypes.c_void_p(find_by_id(w, e1)))
    p.pump(60)
    cur, _cid = focus_of()
    shift_tab(cur)
    back = focus_of()[1]
    check('9 Shift+Tab 反向', back == e3, 'e1 -Shift+Tab-> %s（期望 %s）' % (back, e3))

    # 10) 改 Level 不动 Tab 顺序
    w.setLevel(e1, 9)
    p.pump(60)
    u.SetFocus(ctypes.c_void_p(find_by_id(w, e1)))
    p.pump(60)
    cur, _cid = focus_of()
    tab(cur)
    nxt = focus_of()[1]
    check('10 改 Level 不影响 Tab 顺序', nxt == bTop,
          'e1(Level=9) -> %s（期望 %s）' % (nxt, bTop))

    # 11) 回读一致
    check('11 getLevel / getTabIndex 回读',
          w.getLevel(e1) == 9 and w.getTabIndex(e2) == 1,
          'e1.Level=%d e2.Tab=%d' % (w.getLevel(e1), w.getTabIndex(e2)))

    # 12) 隐藏控件不进 Tab 队列：藏掉 btnBottom，e1 → btnTop → btnSwap
    hb = find_by_id(w, bBottom)
    u.ShowWindow(ctypes.c_void_p(hb), 0)     # SW_HIDE
    p.pump(80)
    u.SetFocus(ctypes.c_void_p(find_by_id(w, e1)))
    p.pump(60)
    cur, _cid = focus_of()
    tab(cur)
    first = focus_of()[1]
    cur, _cid = focus_of()
    tab(cur)
    second = focus_of()[1]
    u.ShowWindow(ctypes.c_void_p(hb), 5)     # SW_SHOW
    check('12 隐藏控件被 Tab 跳过', first == bTop and second == bSwap,
          'e1 -> %s -> %s（跳过隐藏的 btnBottom）' % (first, second))

    ok = sum(1 for r in RESULTS if r[1])
    print('\n%d/%d passed' % (ok, len(RESULTS)))
    if ok == len(RESULTS):
        print('LEVEL TAB TEST PASSED')
    else:
        print('LEVEL TAB TEST FAILED')
    return 0 if ok == len(RESULTS) else 1


if __name__ == '__main__':
    raise SystemExit(main())


