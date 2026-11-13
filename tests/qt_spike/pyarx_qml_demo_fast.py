# -*- coding: utf-8 -*-
"""CAD 内快速自检：不 sleep，用 pump() 推进事件循环，记录首帧真实耗时"""
import sys
import time

sys.path.insert(0, r'G:\PyArx\PyArxQml')
sys.path.insert(0, r'G:\PyArx\tests\qt_spike')

LOG = r'G:\PyArx\docs\_pyarx_qml_fast.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[fast] %s\n' % msg)


log('=== start ===')
import PyArxQml
from pyarx_qml import show

t0 = time.perf_counter()
panel = show(r'G:/PyArx/PyArxQml/qml/panel_demo.qml', name='QT-QML',
             width=340, height=620, log_path=LOG)
log('show() 返回: %.1f ms, hwnd=0x%X' % ((time.perf_counter() - t0) * 1000, panel.hwnd))

# 关键：不 sleep，反复 pump 推事件循环
t1 = time.perf_counter()
frames, loops = 0, 0
while loops < 2000:
    PyArxQml.pump()
    loops += 1
    frames = PyArxQml.stats().get('frames', 0)
    if frames >= 1:
        break
log('首帧: %.1f ms（pump=%d frames=%d）' % ((time.perf_counter() - t1) * 1000, loops, frames))
log('stats=%s' % PyArxQml.stats())

panel.push('PyArxQml 就绪')
for _ in range(30):
    PyArxQml.pump()
time.sleep(0)  # 不阻塞泵
panel.grab(r'G:/PyArx/docs/_pyarx_qml_fast.png')
log('抓图完成')
