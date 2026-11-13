# -*- coding: utf-8 -*-
"""在 CAD 侧栏里用 Qt Quick(QML) 渲染界面。"""
import sys
sys.path.insert(0, r'G:\PyArx\tests\qt_spike')

LOG = r'G:\PyArx\docs\_qtquick_in_cad.log'
NAME = 'QT-QUICK'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[embed] %s\n' % msg)


log('step1 import')
import QtQuickSpike
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

ok = QtQuickSpike.run_qml(r'G:/PyArx/tests/qt_spike/qml_demo.qml', hwnd, 340, 620)
log('step7 run_qml -> %s' % ok)
print('[qtquick] hwnd=0x%X run=%s' % (hwnd, ok))
