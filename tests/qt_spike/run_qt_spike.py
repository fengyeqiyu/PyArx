# -*- coding: utf-8 -*-
"""在 CAD 里启动 QtWebEngine（独立窗口形态）。"""
import sys
sys.path.insert(0, r'G:\PyArx\tests\qt_spike')
import QtSpike
ok = QtSpike.run(r'G:/PyArx/tests/cad/webview_status.html', 0, 340, 700)
print('[qt_spike] run ->', ok)
