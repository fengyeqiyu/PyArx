# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
import PyMfcWebView as w

x = w.WebWindow(400, 300, 't')
print('created', x.hwnd(), flush=True)
w.runFor(3000)
print('ready', x.isReady(), flush=True)
x.destroy()
print('destroyed', flush=True)
