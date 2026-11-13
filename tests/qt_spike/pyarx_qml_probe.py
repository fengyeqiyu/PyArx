# -*- coding: utf-8 -*-
"""脚本返回后（CAD 消息循环恢复）再读一次泵统计，确认 WM_TIMER 在被派发。"""
import sys
sys.path.insert(0, r'G:\PyArx\PyArxQml')

LOG = r'G:\PyArx\docs\_pyarx_qml_demo.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[probe] %s\n' % msg)


import PyArxQml

log('stats = %s' % PyArxQml.stats())
log('result = %s' % PyArxQml.query('result'))
