# -*- coding: utf-8 -*-
"""验证 QML ↔ C++ ↔ Python 双向桥：点 QML 按钮，Python 算 1234+5678，结果回到 QML。"""
import ctypes
import time
import sys

sys.path.insert(0, r'G:\PyArx\tests\qt_spike')

LOG = r'G:\PyArx\docs\_qtquick_bridge.log'
NAME = 'QT-QUICK'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[bridge] %s\n' % msg)


log('step1 import')
import QtQuickSpike
from PyArxCore import Ap
log('step2 import 完成')

pal = Ap.getPalette(NAME, 340, 620)
page = pal.addPage(NAME)
pal.show()
hwnd_page = page.hwnd
log('step3 侧栏就绪 page=0x%X' % hwnd_page)

# Python 侧真正干活的地方：被 C++ 从 QML 调用
calls = []


def add(a, b):
    calls.append((a, b))
    log('step4 Python 收到调用 add(%d, %d)' % (a, b))
    return '%d + %d = %d' % (a, b, a + b)


QtQuickSpike.set_callback(add)
log('step5 回调已注册')

ok = QtQuickSpike.run_qml(r'G:/PyArx/tests/qt_spike/qml_bridge.qml', hwnd_page, 340, 620)
log('step6 run_qml -> %s' % ok)
if not ok:
    raise SystemExit(1)

QtQuickSpike.push('Python 已就绪，等按钮')
log('step7 已推送状态到 QML')

# 点 QML 按钮：按钮矩形在 (12,96)-(328,140)，取中心 (170,118)
time.sleep(1.0)
qt_hwnd = QtQuickSpike.hwnd()
log('step8 QQuickView hwnd=0x%X' % qt_hwnd)

user32 = ctypes.windll.user32
lparam = (118 << 16) | 170
t0 = time.perf_counter()
user32.PostMessageW(qt_hwnd, 0x0200, 0, lparam)   # WM_MOUSEMOVE
user32.PostMessageW(qt_hwnd, 0x0201, 1, lparam)   # WM_LBUTTONDOWN
time.sleep(0.05)
user32.PostMessageW(qt_hwnd, 0x0202, 0, lparam)   # WM_LBUTTONUP
log('step9 已投递点击 (170,118)')

# 轮询结果属性，测量"点击 -> 文本更新"的延迟
result = None
deadline = time.perf_counter() + 3.0
while time.perf_counter() < deadline:
    r = QtQuickSpike.query('result')
    if r and r != '等待 Python':
        result = r
        break
    time.sleep(0.01)
dt = (time.perf_counter() - t0) * 1000

log('step10 界面文本 = %s（点击到更新 %.1f ms，Python 被调 %d 次）' % (result, dt, len(calls)))
print('[bridge] result=%s latency=%.1fms calls=%d' % (result, dt, len(calls)))

time.sleep(0.5)
QtQuickSpike.grab()
log('step11 抓图完成')
