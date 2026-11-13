# -*- coding: utf-8 -*-
"""二分探针：只构造 QGuiApplication，不建视图。用 PYARX_TEST_QPA 切换平台插件后端"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\PyArxQml')
LOG = r'G:\PyArx\docs\_pyarx_qml_probe.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[probe] %s\n' % msg)


qpa = os.environ.get('PYARX_TEST_QPA', 'windows')
os.environ['QT_QPA_PLATFORM'] = qpa
log('=== 探针启动 platform=%s' % qpa)

import PyArxQml
PyArxQml.set_log(LOG)

t0 = time.perf_counter()
PyArxQml.ensure()
log('ensure() = %.1f ms' % ((time.perf_counter() - t0) * 1000))
log('stats=%s' % PyArxQml.stats())
