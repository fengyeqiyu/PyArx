# -*- coding: utf-8 -*-
"""CAD 内只开浮窗然后立刻返回（不 pump），用来验证回到 CAD 后窗口是否自己刷新"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\PyArxQml')
os.add_dll_directory(r'G:\PyArx\X64\Release')

import power_plan_host as host

t0 = time.time()
host.open_floating(log_path=r'G:\PyArx\docs\_pyarx_qml_powerplan.log')
host.log('已开浮窗并立即返回（未 pump），耗时 %.0f ms' % ((time.time() - t0) * 1000))
