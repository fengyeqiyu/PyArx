# -*- coding: utf-8 -*-
"""CAD 进程内：开浮窗后自己 pump 1 秒并连续平移，量进程内真实帧率"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\PyArxQml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import power_plan_host as host
import PyArxQml

LOG = r'G:\PyArx\docs\_pyarx_qml_powerplan.log'
hwnd = host.open_floating(log_path=LOG)


def pump(sec):
    t = time.time()
    while time.time() - t < sec:
        PyArxQml.pump()
        time.sleep(0.004)


pump(1.0)
host.log('CAD 内 render_backend=%s stats=%s' % (PyArxQml.render_backend(), PyArxQml.stats()))

f0 = PyArxQml.stats()['frames']
n = 0
t1 = time.time()
while time.time() - t1 < 2.0:
    PyArxQml.eval("(function(){ tx = tx - 4; return 1; })()")
    PyArxQml.pump()
    time.sleep(0.004)
    n += 1
f1 = PyArxQml.stats()['frames']
host.log('CAD 内平移 2 秒：循环 %d 次，交换帧 %d（%.1f fps）' % (n, f1 - f0, (f1 - f0) / 2.0))

f0 = f1
n = 0
k = 0.2
t1 = time.time()
while time.time() - t1 < 2.0:
    k = 0.2 + 0.3 * ((n % 30) / 30.0)
    PyArxQml.eval("(function(){ k = %f; return 1; })()" % k)
    PyArxQml.pump()
    time.sleep(0.004)
    n += 1
f1 = PyArxQml.stats()['frames']
host.log('CAD 内缩放 2 秒：循环 %d 次，交换帧 %d（%.1f fps）' % (n, f1 - f0, (f1 - f0) / 2.0))
host.log('grab=%s' % PyArxQml.grab(r'G:\PyArx\docs\_cad_powerplan_floating.png'))
