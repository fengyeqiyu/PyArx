# -*- coding: utf-8 -*-
"""验证 power_plan_host.open_standalone 独立窗口入口：起子进程 → 找到窗口 → 关窗"""
import ctypes
import ctypes.wintypes as wt
import subprocess
import sys
import time

PY = r'D:\Program Files\Python314\python.exe'
HOST = r'G:\PyArx\PyArxQml\power_plan_host.py'

proc = subprocess.Popen([PY, HOST, 'standalone'], cwd=r'G:\PyArx\PyArxQml',
                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
user32 = ctypes.windll.user32
user32.FindWindowW.restype = wt.HWND
user32.FindWindowW.argtypes = [wt.LPCWSTR, wt.LPCWSTR]

hwnd = None
t0 = time.time()
while time.time() - t0 < 15:
    hwnd = user32.FindWindowW('PyArxPowerPlanHost', '配电干线图')
    if hwnd:
        break
    time.sleep(0.3)

print('窗口句柄 =', hwnd, '找到耗时 %.1f s' % (time.time() - t0))
if hwnd:
    time.sleep(4.0)          # 让 app 把示例载入并完成首帧
    user32.PostMessageW(hwnd, 0x0010, 0, 0)   # WM_CLOSE
    time.sleep(2.0)

rc = proc.poll()
print('子进程退出码 =', rc if rc is not None else '仍在运行')
if rc is None:
    proc.kill()
out, err = proc.communicate(timeout=10)
print('--- stdout ---'); print(out.decode('utf-8', 'replace')[-800:])
print('--- stderr ---'); print(err.decode('utf-8', 'replace')[-800:])
