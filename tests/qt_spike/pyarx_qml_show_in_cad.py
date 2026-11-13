# -*- coding: utf-8 -*-
"""CAD 进程内验证：不预热，直接 show() 打开 QML 面板（记录真实耗时）"""
import sys
import time

sys.path.insert(0, r'G:\PyArx\PyArxQml')
LOG = r'G:\PyArx\docs\_pyarx_qml_show_cad.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[show] %s\n' % msg)


log('=== 开始（CAD 进程内，直接 show，不预热）')
from pyarx_qml import show

t = time.perf_counter()
panel = show(r'G:/PyArx/PyArxQml/qml/panel_demo.qml', name='QT-QML',
             width=340, height=620, log_path=LOG)
log('show() 耗时 %.1f ms（面板 hwnd=0x%X）' % ((time.perf_counter() - t) * 1000, panel.hwnd))

panel.set('pyResult', '直接 show 打开，没有预热')
log('set_prop 回读 = %r' % panel.get('pyResult'))

import PyArxQml
for _ in range(60):
    PyArxQml.pump()
panel.grab(r'G:/PyArx/docs/_pyarx_qml_show_cad.png')
log('抓图完成，总耗时 %.1f ms' % ((time.perf_counter() - t) * 1000))
