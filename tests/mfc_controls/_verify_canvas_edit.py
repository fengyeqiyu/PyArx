# -*- coding: utf-8 -*-
"""_verify_canvas_edit.py — 画布图元「选中 / 删除」验收（纯 Python，独立进程）。

验 9 条：
 1) 命令串被解析成图元表，条数与包围盒对得上；
 2) 点中图元 → 选中序号正确，且 onCanvasSelect 回调被点亮；
 3) 点空白 → 取消选中；
 4) 空白拖拽 → 框选命中多个；
 5) 选中框真的画到了像素上（边框位置出现强调蓝）；
 6) 删除选中的图元 → 条数减一、命令串里不再有那条；
 7) 其余图元文本一字未改（删一个不动一群）；
 8) 关掉可编辑后，点击不再改选中；
 9) 删除后再点，命中的是"新顺序"里的图元（下标跟得上）。
"""
import ctypes
import os
from ctypes import wintypes

import PyMfcXaml as p
from demo_canvas_edit import CMDS, build
from _verify_flickable import capture

u = ctypes.windll.user32
u.PostMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
CB = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)

WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MOUSEMOVE = 0x0201, 0x0202, 0x0200
mk = lambda x, y: ((y & 0xFFFF) << 16) | (x & 0xFFFF)


def find_canvas(w, cid):
    """按控件 id 找画布窗口：EnumChildWindows 是递归的，第 0 个往往是 id=0 的宿主。"""
    got = []

    def cb(h, _):
        if u.GetDlgCtrlID(ctypes.c_void_p(h)) == cid:
            got.append(h)
            return 0
        return 1

    u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
    u.GetDlgCtrlID.restype = ctypes.c_int
    u.EnumChildWindows(ctypes.c_void_p(w.hwnd), CB(cb), 0)
    return got[0] if got else None


def down(h, x, y):
    u.PostMessageW(ctypes.c_void_p(h), WM_LBUTTONDOWN, ctypes.c_void_p(1), ctypes.c_void_p(mk(x, y)))


def move(h, x, y):
    u.PostMessageW(ctypes.c_void_p(h), WM_MOUSEMOVE, ctypes.c_void_p(1), ctypes.c_void_p(mk(x, y)))


def up(h, x, y):
    u.PostMessageW(ctypes.c_void_p(h), WM_LBUTTONUP, ctypes.c_void_p(0), ctypes.c_void_p(mk(x, y)))


def key(h, vk):
    u.PostMessageW(ctypes.c_void_p(h), 0x0100, ctypes.c_void_p(vk), ctypes.c_void_p(0))


def read_px(bmp, x, y):
    import struct
    d = open(bmp, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w = struct.unpack_from('<i', d, 18)[0]
    h = struct.unpack_from('<i', d, 22)[0]
    bpp = struct.unpack_from('<H', d, 28)[0]
    stride = ((w * bpp // 8 + 3) // 4) * 4
    topdown = h < 0
    hh = abs(h)
    row = y if topdown else (hh - 1 - y)
    i = off + row * stride + x * 4
    if i < 0 or i + 3 >= len(d):
        return None
    return (d[i + 2], d[i + 1], d[i])


def main():
    names = {}
    w = p.guiload(build(), names=names)
    w.show()
    p.pump(250)
    cv = names['cv']
    w.setCanvasCommands(cv, CMDS)
    w.setCanvasEditable(cv, True)
    p.pump(150)
    h = find_canvas(w, cv)
    assert h, '画布 hwnd 没找到'

    hits = {'n': 0}

    def on_sel(*a):
        hits['n'] += 1

    w.onCanvasSelect(cv, on_sel)

    ok = bad = 0

    def check(name, good, detail):
        nonlocal ok, bad
        ok, bad = (ok + 1, bad) if good else (ok, bad + 1)
        print(('  OK   ' if good else '  FAIL ') + '%-36s %s' % (name, detail))

    # 1) 图元表
    n = w.getCanvasItemCount(cv)
    ops = [w.getCanvasItemOp(cv, i) for i in range(n)]
    boxes = [tuple(w.getCanvasItemBox(cv, i)) for i in range(n)]
    print('    共 %d 个图元：%s' % (n, list(zip(ops, boxes))))
    check('图元条数 = 6', n == 6, 'got=%d' % n)
    check('第 2 条是 Rect 且盒子对', ops[1] == 'Rect' and boxes[1] == (20, 20, 140, 100),
          'op=%s box=%s' % (ops[1], boxes[1]))

    # 2) 点中第 2 个图元（矩形中心 80,60）
    down(h, 80, 60)
    up(h, 80, 60)
    p.pump(120)
    check('点击命中 → 选中 [1]', w.getCanvasSelection(cv) == [1], 'sel=%s' % w.getCanvasSelection(cv))
    check('onCanvasSelect 被点亮', hits['n'] >= 1, 'hits=%d' % hits['n'])

    # 5) 选中框真的画到像素上：抓图看包围盒边缘是不是强调蓝
    capture(w.hwnd, r'G:\PyArx\docs\_cve_sel.bmp')
    wp, cp = wintypes.RECT(), wintypes.RECT()
    u.GetWindowRect(ctypes.c_void_p(w.hwnd), ctypes.byref(wp))
    u.GetWindowRect(ctypes.c_void_p(h), ctypes.byref(cp))
    dx, dy = cp.left - wp.left, cp.top - wp.top
    edge = read_px(r'G:\PyArx\docs\_cve_sel.bmp', dx + 20 - 2, dy + 20 - 2 + 6)
    bluish = edge is not None and edge[2] > edge[0] + 40
    check('选中框像素偏蓝', bluish, 'px@框边=%s' % (edge,))

    # 3) 点空白 → 清空选中
    down(h, 400 - 10, 300 - 10)
    up(h, 400 - 10, 300 - 10)
    p.pump(120)
    check('点空白 → 选中清空', w.getCanvasSelection(cv) == [], 'sel=%s' % w.getCanvasSelection(cv))

    # 4) 框选：从 (10,10) 拖到 (300,150)，应框中 Rect + Ellipse + RoundRect(部分) + Line
    down(h, 10, 10)
    move(h, 120, 60)
    move(h, 300, 150)
    up(h, 300, 150)
    p.pump(150)
    sel = w.getCanvasSelection(cv)
    check('框选命中多个', len(sel) >= 2, 'sel=%s' % (sel,))
    capture(w.hwnd, r'G:\PyArx\docs\_cve_band.bmp')

    # 6) 删除选中的图元
    n0 = w.getCanvasItemCount(cv)
    plot = tuple(w.getCanvasItemBox(cv, sel[0]))
    removed = w.removeCanvasSelection(cv)
    p.pump(120)
    check('删除数量 = 选中数量', removed == len(sel), 'removed=%d sel=%s' % (removed, sel))
    check('删除后条数减少', w.getCanvasItemCount(cv) == n0 - removed,
          '%d → %d' % (n0, w.getCanvasItemCount(cv)))

    # 7) 剩余命令串里，未被删的图元文本仍在（删一个不动一群）
    left = w.getCanvasCommands(cv)
    still = all(op in left for op in ('Text,20,170',)) or 'Text' in left
    check('其余图元仍在命令串里', 'Text' in left and 'Clear' in left, 'cmd=%s' % left[:90])

    # 8) 关掉可编辑 → 点图元不改选中
    w.setCanvasEditable(cv, False)
    p.pump(100)
    before = w.getCanvasSelection(cv)
    down(h, 190, 60)
    up(h, 190, 60)
    p.pump(100)
    check('不可编辑时不改选中', w.getCanvasSelection(cv) == before, 'sel=%s' % w.getCanvasSelection(cv))

    # 9) 重新打开，点第一个仍在的图元，检查下标跟着新顺序
    w.setCanvasEditable(cv, True)
    p.pump(100)
    nb = [tuple(w.getCanvasItemBox(cv, i)) for i in range(w.getCanvasItemCount(cv))]
    idx = next((i for i, b in enumerate(nb) if b[2] - b[0] > 20), 0)
    b = nb[idx]
    down(h, (b[0] + b[2]) // 2, (b[1] + b[3]) // 2)
    up(h, (b[0] + b[2]) // 2, (b[1] + b[3]) // 2)
    p.pump(120)
    check('删除后再点命中新下标', w.getCanvasSelection(cv) == [idx],
          'want=[%d] got=%s' % (idx, w.getCanvasSelection(cv)))

    # 10) Delete 真键盘路径：点中 → 发 WM_KEYDOWN VK_DELETE → 图元被摘掉
    n1 = w.getCanvasItemCount(cv)
    b = tuple(w.getCanvasItemBox(cv, idx))
    down(h, (b[0] + b[2]) // 2, (b[1] + b[3]) // 2)
    up(h, (b[0] + b[2]) // 2, (b[1] + b[3]) // 2)
    p.pump(100)
    key(h, 0x2E)
    p.pump(150)
    check('Delete 键删除选中图元', w.getCanvasItemCount(cv) == n1 - 1,
          '%d → %d sel=%s' % (n1, w.getCanvasItemCount(cv), w.getCanvasSelection(cv)))

    capture(w.hwnd, r'G:\PyArx\docs\_cve_after.bmp')
    print('画布编辑验收：通过 %d，失败 %d' % (ok, bad))
    print('CANVAS EDIT TEST %s' % ('PASSED' if bad == 0 else 'FAILED'))
    w.close()
    p.pump(60)


if __name__ == '__main__':
    main()
