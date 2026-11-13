# -*- coding: utf-8 -*-
"""装自检页侧栏：页面在收到 shell.ready 后自己发消息（CAD 空闲时才收到回调）。"""
import sys
import time

LOG = r'G:\PyArx\docs\_isolate2.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('%s %s\n' % (time.strftime('%H:%M:%S'), msg))


log('装自检页侧栏')
from PyArxCore import Ap  # noqa: E402

pal = Ap.loadPalette(r'G:\PyArx\tests\cad\palette_selftest.html', 'PY-SELF', 340, 560)
log('loadPalette -> %r' % (pal,))
setattr(sys, '_selftest_pal', pal)
log('装完（外部等 40 秒，让空闲消息循环去处理页面的 post）')
