# -*- coding: utf-8 -*-
"""把 QtWebEngineView 塞进 CAdUiPalette 的页窗口（侧栏形态，真机）。

顺序刻意先建侧栏（纯 MFC），再初始化 Qt，避免两个系统初始化顺序打架。
每一步都落日志，便于崩溃后定位最后一步。
"""
import sys
sys.path.insert(0, r'G:\PyArx\tests\qt_spike')

LOG = r'G:\PyArx\docs\_qt_in_cad.log'
NAME = 'QT-EMBED'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[embed] %s\n' % msg)


log('step1 准备 import')
import QtSpike
from PyArxCore import Ap
log('step2 import 完成')

pal = Ap.getPalette(NAME, 340, 620)
log('step3 getPalette 完成')
page = pal.addPage(NAME)
log('step4 addPage 完成')
pal.show()
log('step5 show 完成')
hwnd = page.hwnd
log('step6 page hwnd = 0x%X' % hwnd)

ok = QtSpike.run(r'G:/PyArx/tests/cad/webview_status.html', hwnd, 340, 620)
log('step7 QtSpike.run -> %s' % ok)
print('[embed] hwnd=0x%X run=%s' % (hwnd, ok))
