# -*- coding: utf-8 -*-
"""只加载停靠侧栏（被动页面，不会自动发消息），每步落地成日志以防崩溃丢证据。"""
import sys

LOG = r'G:\PyArx\docs\_pal_only.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


log('=== 步骤 1：准备加载 palette.html ===')
from PyArxCore import Ap  # noqa: E402

log('步骤 2：Ap 已导入，开始 loadPalette')
pal = Ap.loadPalette(r'G:\PyArx\X64\Release\palette.html', 'PY-WEB', 340, 560)
log('步骤 3：loadPalette 返回 %r' % (pal,))
setattr(sys, '_py_pal_only', pal)
log('=== 步骤 4：加载完成，等 30 秒看是否崩 ===')
