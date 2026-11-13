# -*- coding: utf-8 -*-
"""不建侧栏的最小用例：只验证 arx 已载入、Python 能跑、print 的去向是哪。"""
import sys

LOG = r'G:\PyArx\docs\_no_pal.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


log('=== 开始：只跑 python，不建侧栏 ===')
log('sys.stdout 类型 = %s.%s' % (type(sys.stdout).__module__, type(sys.stdout).__name__))

print('print 到命令行：最小用例第一行')
print('print 到命令行：最小用例第二行')

import PyArxCore  # noqa: E402

log('PyArxCore 导入成功：%r' % (PyArxCore,))
log('PyArxCore.Ap 存在 = %s' % hasattr(PyArxCore, 'Ap'))

try:
    from PyArxCore import Ap
    log('Ap 模块 = %r' % (Ap,))
    log('Ap 里 loadPalette = %s' % hasattr(Ap, 'loadPalette'))
except Exception as e:
    log('Ap 导入异常：%r' % (e,))

log('=== 结束（没崩溃说明这条链路是稳的） ===')
