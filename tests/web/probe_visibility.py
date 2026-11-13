# -*- coding: utf-8 -*-
"""探针：确认 WebWindow 建出来后是否真的可见，并打印客户区尺寸。"""
import ctypes
import sys
import time

sys.path.insert(0, r'G:\PyArx\X64\Release')
import PyMfcWebView as wv

LOG = r'G:\PyArx\docs\_pywebview_probe.log'


def log(m):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(m) + '\n')


open(LOG, 'w', encoding='utf-8').close()

w = wv.WebWindow(900, 640, 'PyArx 独立窗体')
log('hwnd=%s' % w.hwnd())
w.loadFile(r'G:\PyArx\tests\web\pywebview_demo.html')
w.show()
w.center()
w.runFor(400)

u = ctypes.windll.user32
h = w.hwnd()
log('IsWindowVisible=%s IsWindow=%s' % (u.IsWindowVisible(h), u.IsWindow(h)))
r = ctypes.wintypes.RECT() if hasattr(ctypes, 'wintypes') else None
import ctypes.wintypes as wt
rc = wt.RECT()
u.GetWindowRect(h, ctypes.byref(rc))
log('rect=%s,%s %sx%s' % (rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top))
u.ShowWindow(h, 5)
log('after explicit ShowWindow: visible=%s' % u.IsWindowVisible(h))
w.runFor(3000)
log('ready=%s closed=%s' % (w.isReady(), w.isClosed()))
w.destroy()
log('done')
