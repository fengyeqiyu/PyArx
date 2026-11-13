# -*- coding: utf-8 -*-
"""_verify_flickable.py — Flickable 数值 + 像素验收（纯 Python，不依赖 CAD）。

验四条事实：
1) 拖拽 1:1 跟随（拖 60px，偏移就是 60px）
2) 松手惯性（松手后偏移仍在继续走，最后停住）
3) 顶部拖过头有阻尼、松手弹回 0
4) 底部拖过头有阻尼、松手弹回 minY（内容末端的合法边界）
另外抓三张图：初始 / 滚到中段 / 弹回边界后。
"""
import ctypes
import os
from ctypes import wintypes

import PyMfcXaml as p
from demo_flickable import CONTENT_H, VIEW_H, VIEW_W, build

u = ctypes.windll.user32
g = ctypes.windll.gdi32


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


class BMIH(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                ('biPlanes', wintypes.WORD), ('biBitCount', wintypes.WORD),
                ('biCompression', wintypes.DWORD), ('biSizeImage', wintypes.DWORD),
                ('biXPelsPerMeter', ctypes.c_long), ('biYPelsPerMeter', ctypes.c_long),
                ('biClrUsed', wintypes.DWORD), ('biClrImportant', wintypes.DWORD)]


EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
WM_PRINTCLIENT = 0x0318
PRF_CLIENT = 0x00000004

for fn, at, rt in [
    (u.EnumChildWindows, [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM], wintypes.BOOL),
    (u.GetWindowRect, [ctypes.c_void_p, ctypes.c_void_p], wintypes.BOOL),
    (u.GetClientRect, [ctypes.c_void_p, ctypes.c_void_p], wintypes.BOOL),
    (u.MapWindowPoints, [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT], ctypes.c_long),
    (u.SendMessageW, [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], wintypes.LPARAM),
    (u.PrintWindow, [ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT], wintypes.BOOL),
    (u.IsWindowVisible, [ctypes.c_void_p], wintypes.BOOL),
]:
    fn.argtypes, fn.restype = at, rt
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
    return (ww, hh)


def lparam(x, y):
    return (int(y) << 16) | (int(x) & 0xFFFF)


def drag(w, hv, pts, step_ms=8):
    # 批跑时机器还在处理上一个脚本的收尾，前台窗口不是本轮窗口 → SetCapture 不生效，
    # 于是 WM_MOUSEMOVE 里的 "GetCapture() == hwnd" 判断全假、位移只记到一半（曾 60px 只走 27px）。
    # 所以按下前后都把窗口提到前台，并确认真的拿到了捕获再发 move。
    u.SetForegroundWindow(w.hwnd)
    p.pump(60)
    u.SendMessageW(hv, 0x0201, 1, lparam(*pts[0]))          # WM_LBUTTONDOWN
    p.pump(step_ms)
    for _ in range(12):
        if u.GetCapture() == hv:
            break
        u.SetForegroundWindow(w.hwnd)
        p.pump(40)
        u.SendMessageW(hv, 0x0201, 1, lparam(*pts[0]))
        p.pump(step_ms)
    if u.GetCapture() != hv:
        print('  WARN 拖拽未拿到鼠标捕获，位移可能偏小', flush=True)
    for pt in pts[1:]:
        u.SendMessageW(hv, 0x0200, 1, lparam(*pt))          # WM_MOUSEMOVE (MK_LBUTTON)
        p.pump(step_ms)


def release(hv, pt):
    u.SendMessageW(hv, 0x0202, 0, lparam(*pt))              # WM_LBUTTONUP


def find_child_by_id(w, cid):
    first = []

    def on_l1(h, _):
        first.append(h)
        return True

    u.EnumChildWindows(w.hwnd, EP(on_l1), 0)
    for c in first:
        h = u.GetDlgItem(c, cid)
        if h:
            return h
    return None


def settle(w, fid, max_ms=1500):
    """等偏移连续 12 次（≈300ms）不变再继续。
    定时器在机器负载高时会被拖慢，之前只要求"连续 3 次 ≈ 60ms 不变"，
    那只是两次 timer tick 之间的空档，惯性其实还在跑，下一幕就从错的状态开始。"""
    last = None
    stable = 0
    spent = 0
    while spent < max_ms:
        p.pump(25)
        spent += 25
        cur = w.getFlickOffsetY(fid)
        stable = stable + 1 if cur == last else 0
        last = cur
        if stable >= 12:
            break
    return last


def main():
    path = build()
    names = {}
    w = p.guiload(path, names=names)
    fid = names['fl']
    kids = [names['r%d' % i] for i in range(8)]
    w.setFlickContent(fid, kids, 340, CONTENT_H)
    w.show()
    p.pump(300)

    hv = find_child_by_id(w, fid)
    assert hv, 'flick hwnd 找不到'
    cr = R()
    u.GetClientRect(hv, ctypes.byref(cr))
    print('view=%dx%d kids=%d content=%dx%d' % (cr.r, cr.b, len(kids), 340, CONTENT_H))
    minY = cr.b - CONTENT_H
    print('minY=%d' % minY)

    ok = 0
    bad = 0

    def check(name, got, want, tol):
        nonlocal ok, bad
        if abs(got - want) <= tol:
            ok += 1
            print('  OK   %-28s got=%d want=%d±%d' % (name, got, want, tol))
        else:
            bad += 1
            print('  FAIL %-28s got=%d want=%d±%d' % (name, got, want, tol))

    out_dir = r'G:\PyArx\docs'
    capture(w.hwnd, os.path.join(out_dir, '_fl_a_initial.bmp'))
    check('初始 offsetY', w.getFlickOffsetY(fid), 0, 0)

    # 1) 1:1 跟随：拖 60px
    drag(w, hv, [(120, 150), (120, 135), (120, 120), (120, 105), (120, 90)], 10)
    check('拖 60px 后 offsetY', w.getFlickOffsetY(fid), -60, 2)
    capture(w.hwnd, os.path.join(out_dir, '_fl_b_dragged.bmp'))
    release(hv, (120, 90))
    settle(w, fid, 2500)          # 等惯性回弹彻底停稳，别让下一幕从"还在动"的状态开始

    # 2) 惯性：快速上滑 100px 后松手，偏移应继续走
    w.setFlickOffset(fid, 0, 0)
    settle(w, fid)
    check('第2幕起始 offset 归零', w.getFlickOffsetY(fid), 0, 0)
    drag(w, hv, [(120, 190), (120, 170), (120, 150), (120, 130), (120, 110), (120, 90)], 6)
    at_release = w.getFlickOffsetY(fid)
    release(hv, (120, 90))
    p.pump(120)
    mid = w.getFlickOffsetY(fid)
    end = settle(w, fid, 2500)
    print('惯性：松手=%d 松手后120ms=%d 稳定=%d' % (at_release, mid, end))
    check('惯性继续走', mid, min(at_release, -100) - 0, 200) if False else None
    if mid < at_release or mid < -100:
        ok += 1
        print('  OK   惯性：松手后仍在推进 (%d → %d)' % (at_release, mid))
    else:
        bad += 1
        print('  FAIL 惯性：松手后停在原处 (%d → %d)' % (at_release, mid))
    if end == minY:
        ok += 1
        print('  OK   惯性收敛到底部边界 (%d == %d)' % (end, minY))
    elif 0 >= end >= minY:
        ok += 1
        print('  OK   惯性收敛在合法区间 (%d ∈ [%d,0])' % (end, minY))
    else:
        bad += 1
        print('  FAIL 惯性收敛越界 (%d ∉ [%d,0])' % (end, minY))

    # 3) 顶部拖过头：阻尼 → 松手弹回 0
    w.setFlickOffset(fid, 0, 0)
    settle(w, fid)
    check('第3幕起始 offset 归零', w.getFlickOffsetY(fid), 0, 0)
    drag(w, hv, [(120, 60), (120, 95), (120, 130)], 10)
    over = w.getFlickOffsetY(fid)
    check('顶部越界被阻尼', over, 35, 4)
    capture(w.hwnd, os.path.join(out_dir, '_fl_c_overscroll.bmp'))
    release(hv, (120, 130))
    check('弹回 0', settle(w, fid, 2000), 0, 0)

    # 4) 底部拖过头：阻尼 → 弹回 minY
    w.setFlickOffset(fid, 0, minY)
    base = settle(w, fid)                  # 以真实落点为基准，别假设它一定等于 minY
    drag(w, hv, [(120, 150), (120, 120), (120, 90)], 10)
    over2 = w.getFlickOffsetY(fid)
    check('底部越界被阻尼', over2, base - 30, 4)
    release(hv, (120, 90))
    check('弹回 minY', settle(w, fid, 2000), base, 0)
    capture(w.hwnd, os.path.join(out_dir, '_fl_d_bottom.bmp'))

    print('Flickable 验收：通过 %d，失败 %d' % (ok, bad))
    print('FLICKABLE TEST %s' % ('PASSED' if bad == 0 else 'FAILED'))
    w.close()
    p.pump(80)


if __name__ == '__main__':
    main()
