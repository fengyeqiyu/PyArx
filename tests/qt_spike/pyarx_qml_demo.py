# -*- coding: utf-8 -*-
"""PyArxQml 模块端到端自检：建侧栏 -> 挂 QML -> 点按钮 -> Python 算 -> 文本回写 -> 抓图。"""
import ctypes
import sys
import time

sys.path.insert(0, r'G:\PyArx\PyArxQml')
sys.path.insert(0, r'G:\PyArx\tests\qt_spike')

LOG = r'G:\PyArx\docs\_pyarx_qml_demo.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[demo] %s\n' % msg)


log('step1 import PyArxQml')
import PyArxQml
from pyarx_qml import show
log('step2 import 完成')

calls = []


def handler(op, a, b):
    calls.append((op, a, b))
    log('step3 Python 收到 %s(%d, %d)' % (op, a, b))
    if op == 'add':
        return '%d + %d = %d' % (a, b, a + b)
    return 'ok'


PyArxQml.set_callback(handler)
log('step4 回调已注册')

panel = show(r'G:/PyArx/PyArxQml/qml/panel_demo.qml', name='QT-QML', width=340, height=620, log_path=LOG)
log('step5 面板已挂载 hwnd=0x%X' % panel.hwnd)

panel.push('PyArxQml 已就绪')

# 点按钮（面板坐标固定：按钮 (12,96)-(328,140)，中心 170,118）
time.sleep(2.0)
qt_hwnd = PyArxQml.hwnd()
user32 = ctypes.windll.user32
lparam = (118 << 16) | 170
user32.PostMessageW(qt_hwnd, 0x0200, 0, lparam)
user32.PostMessageW(qt_hwnd, 0x0201, 1, lparam)
time.sleep(0.05)
user32.PostMessageW(qt_hwnd, 0x0202, 0, lparam)
log('step6 已投递点击')

time.sleep(1.5)
result = panel.query('result')
status = panel.query('status')
stats = panel.stats()
log('step7 result=%s status=%s stats=%s' % (result, status, stats))
print('[demo] result=%s' % result)
print('[demo] status=%s' % status)
print('[demo] stats=%s' % stats)

panel.grab(r'G:/PyArx/docs/_pyarx_qml_demo.png')
log('step8 抓图完成')
