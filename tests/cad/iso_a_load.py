# -*- coding: utf-8 -*-
"""隔离测试：全程不抓图、不点鼠标，只用日志判定每一步是否还活着。

阶段 A：只装侧栏，等 45 秒
阶段 B：页面 post 一条 cad.count，等 15 秒
阶段 C：页面 post 一条 py.exec（带 print），等 15 秒
阶段 D：再跑一次普通脚本，确认命令通道没坏
每阶段结束都写一行带时间戳的日志，崩在哪一段一眼可见。
"""
import time

LOG = r'G:\PyArx\docs\_isolate.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('%s %s\n' % (time.strftime('%H:%M:%S'), msg))


PHASE = r'G:\PyArx\docs\_isolate_phase.txt'

with open(PHASE, 'w', encoding='utf-8') as f:
    f.write('A')

log('阶段 A：装侧栏')
from PyArxCore import Ap  # noqa: E402

pal = Ap.loadPalette(r'G:\PyArx\X64\Release\palette.html', 'PY-WEB', 340, 560)
log('loadPalette -> %r' % (pal,))
setattr(__import__('sys'), '_iso_pal', pal)
log('阶段 A 结束（随后外部等 45 秒看是否还活着）')
