# -*- coding: utf-8 -*-
"""干净测时间：只 loadPalette，立刻把主线程还给 CAD 的消息循环。

上一次测到的 10 秒是假的——脚本自己在主线程里 sleep 了 10 秒，
WebView2 的异步回调（controller 创建）根本没机会被泵到。
"""
import time
from PyArxCore import Ap

HTML = r'G:\PyArx\tests\cad\sum_demo.html'
NAME = 'SUM-TIME'

t0 = time.time()
pal = Ap.loadPalette(HTML, NAME, 320, 480)
print('[timing] loadPalette 返回：%.0f ms，hwnd=%d' % ((time.time() - t0) * 1000, int(pal.hwnd)))
