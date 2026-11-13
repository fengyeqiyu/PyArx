# -*- coding: utf-8 -*-
"""在 CAD 里装「WebView 状态」页，并把宿主侧能查到的状态写进日志。"""
import ctypes
import ctypes.wintypes as wt
import sys
import time

LOG = r'G:\PyArx\docs\_mwv_status.log'
HTML = r'G:\PyArx\tests\cad\webview_status.html'

user32 = ctypes.windll.user32


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('%s %s\n' % (time.strftime('%H:%M:%S'), msg))


def rect_of(hwnd):
    rc = wt.RECT()
    if not user32.GetWindowRect(hwnd, ctypes.byref(rc)):
        return None
    return (rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top)


def klass(hwnd):
    buf = ctypes.create_unicode_buffer(128)
    user32.GetClassNameW(hwnd, buf, 128)
    return buf.value


def chain(hwnd, depth=8):
    out = []
    for _ in range(depth):
        if not hwnd:
            break
        rc = rect_of(hwnd)
        out.append('%s(%s)' % (klass(hwnd), 'none' if rc is None else '%d,%d %dx%d' % rc))
        hwnd = user32.GetParent(hwnd)
    return ' <- '.join(out)


def children(hwnd, depth=0, max_depth=3):
    out = []
    if depth > max_depth:
        return out
    h = user32.GetWindow(hwnd, 5)  # GW_CHILD
    while h:
        rc = rect_of(h)
        vis = user32.IsWindowVisible(h)
        out.append('%s%s [%s] %s' % ('  ' * depth, klass(h), h,
                                     'none' if rc is None else '%d,%d %dx%d' % rc) + ('' if vis else ' (hidden)'))
        out.extend(children(h, depth + 1, max_depth))
        h = user32.GetWindow(h, 2)  # GW_HWNDNEXT
    return out


log('== 状态页装载开始 ==')
from PyArxCore import Ap  # noqa: E402

pal = Ap.loadPalette(HTML, 'PY-SELF', 340, 560)
log('loadPalette -> %r' % (pal,))

hwnd = int(pal.hwnd)
log('page hwnd=%d class=%s rect=%s' % (hwnd, klass(hwnd), rect_of(hwnd)))
log('getHost()=%d （0 表示 WebView 直接挂在页窗口上，没有中间宿主窗）' % int(pal.getHost()))
log('页窗口可见=%d' % user32.IsWindowVisible(hwnd))
log('层级：%s' % chain(hwnd))
log('--- 页窗口子窗 ---')
for line in children(hwnd):
    log(line)

setattr(sys, '_mwv_status_pal', pal)
log('== 装载完成 ==')
