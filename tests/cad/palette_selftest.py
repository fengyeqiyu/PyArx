# -*- coding: utf-8 -*-
"""在 CAD 里挂自检侧栏页：页面收到 shell.ready 后自动发 py.exec，看 print 的去向。"""
import sys

import PyArxCore
from PyArxCore.Ap import loadPalette

pal = loadPalette(r'G:\PyArx\tests\cad\palette_selftest.html', 'PY-SELFTEST', 340, 560)
setattr(sys, '_py_self_test_pal', pal)   # 防回收
print('侧栏已建：%r' % (pal,))
