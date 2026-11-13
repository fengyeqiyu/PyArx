# -*- coding: utf-8 -*-
"""CAD 内验证：配电干线图 QML 面板以独立浮窗打开（不用侧栏页签）"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\PyArxQml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import power_plan_host as host
import PyArxQml

t0 = time.time()
hwnd = host.open_floating(log_path=r'G:\PyArx\docs\_pyarx_qml_powerplan.log')
host.log('open_floating 返回 hwnd=0x%X，耗时 %.0f ms' % (hwnd, (time.time() - t0) * 1000))

# 只在抓图这一段用 pump（平时靠 pyd 的泵定时器驱动，不阻塞 CAD 命令行）
t1 = time.time()
while time.time() - t1 < 2.0:
    PyArxQml.pump()
    time.sleep(0.01)

host.log('stats=%s' % (PyArxQml.stats(),))
host.log('info=%s' % PyArxQml.call('info'))
host.log('grab=%s' % PyArxQml.grab(r'G:\PyArx\docs\_cad_powerplan_floating.png'))

# 再等 2 秒，验证函数返回后、没有脚本 pump 的情况下帧还在继续交换（靠泵定时器）
f0 = PyArxQml.stats().get('frames')
time.sleep(2.0)
f1 = PyArxQml.stats().get('frames')
host.log('脚本不 pump 的 2 秒内帧数 %s -> %s（自驱动泵 %s）'
         % (f0, f1, '有效' if (f1 or 0) > (f0 or 0) else '未推进'))
host.log('全部完成，总耗时 %.0f ms' % ((time.time() - t0) * 1000))
