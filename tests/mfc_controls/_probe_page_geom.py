# -*- coding: utf-8 -*-
"""临时探针：页容器 addPage 之后，子控件到底在哪、可见不可见。"""
import ctypes
from ctypes import wintypes
import PyMfcXaml as p
from demo_page_anim import build

u = ctypes.windll.user32
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetClientRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetDlgItem.argtypes = [ctypes.c_void_p, ctypes.c_int]
u.GetDlgItem.restype = ctypes.c_void_p
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.IsWindowVisible.argtypes = [ctypes.c_void_p]
u.GetParent.argtypes = [ctypes.c_void_p]
u.GetParent.restype = ctypes.c_void_p
CB = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long),
                ('r', ctypes.c_long), ('b', ctypes.c_long)]


def all_children(top):
    got = []

    def on(h, _):
        got.append(h)
        return 1

    u.EnumChildWindows(ctypes.c_void_p(top), CB(on), 0)
    return got


def by_id(top, cid):
    for h in all_children(top):
        h2 = u.GetDlgItem(ctypes.c_void_p(h), cid)
        if h2:
            return h2
    return None


def info(tag, hwnd):
    if not hwnd:
        print('%-6s <无窗口>' % tag, flush=True)
        return
    r = R()
    u.GetWindowRect(ctypes.c_void_p(hwnd), ctypes.byref(r))
    vis = u.IsWindowVisible(ctypes.c_void_p(hwnd))
    par = u.GetParent(ctypes.c_void_p(hwnd))
    print('%-6s hwnd=%s parent=%s rect=(%d,%d,%d,%d) %dx%d visible=%s'
          % (tag, hwnd, par, r.l, r.t, r.r, r.b, r.r - r.l, r.b - r.t, bool(vis)), flush=True)


names = {}
w = p.guiload(build(), names=names)
w.show()
p.pump(400)
print('顶层窗口 hwnd=%s' % w.hwnd, flush=True)
win = R()
u.GetWindowRect(ctypes.c_void_p(w.hwnd), ctypes.byref(win))
print('窗口 rect=(%d,%d,%d,%d)' % (win.l, win.t, win.r, win.b), flush=True)

print('--- addPage 之前 ---', flush=True)
for k in ('sv', 's1', 'p1t', 'p1b', 'p1p'):
    info(k, by_id(w.hwnd, names[k]))

w.addPage(names['sv'], [names['s1'], names['p1t'], names['p1b'], names['p1p']])
p.pump(600)
print('--- addPage 之后 ---', flush=True)
for k in ('sv', 's1', 'p1t', 'p1b', 'p1p'):
    info(k, by_id(w.hwnd, names[k]))

print('--- 视图数/索引 ---', flush=True)
print('views=%d index=%d' % (w.getViewCount(names['sv']), w.getViewIndex(names['sv'])), flush=True)
w.savePng(r'G:\PyArx\docs\_pa_probe.png')
print('saved _pa_probe.png', flush=True)
p.pump(200)

print('--- z 序（EnumChildWindows 返回顺序=从上到下）---', flush=True)
allc = all_children(w.hwnd)
ids = {names[k]: k for k in ('sv','s1','p1t','p1b','p1p')}
for i, h in enumerate(allc):
    cid = u.GetDlgCtrlID(ctypes.c_void_p(h))
    tag = ids.get(cid, '')
    if tag or cid == 0:
        print('  #%d id=%s %s' % (i, cid, tag), flush=True)
w.close()
p.pump(200)
p.shutdown()
