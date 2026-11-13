# -*- coding: utf-8 -*-
"""CAD 内：开浮窗并立刻返回（不 pump），然后靠定时器自驱；再隔 3 秒查帧数"""
import os
import sys
import time
sys.path.insert(0, r'G:\PyArx\PyArxQml')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import power_plan_host as host
import PyArxQml

hwnd = host.open_floating(log_path=r'G:\PyArx\docs\_pyarx_qml_powerplan.log')
host.log('打开返回 hwnd=0x%X，此刻 frames=%s' % (hwnd, PyArxQml.stats().get('frames')))
