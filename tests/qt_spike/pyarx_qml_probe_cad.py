# -*- coding: utf-8 -*-
"""CAD 进程内二分探针：mode = core（只建 QCoreApplication） | gui（完整 QGuiApplication）。
模式从 G:\PyArx\docs\_probe_mode.txt 读，避免依赖 CAD 进程的环境变量。"""
import os
import sys
import time

sys.path.insert(0, r'G:\PyArx\PyArxQml')
LOG = r'G:\PyArx\docs\_pyarx_qml_probe_cad.log'
MODE_FILE = r'G:\PyArx\docs\_probe_mode.txt'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(msg + '\n')


mode = 'gui'
extras = {}
try:
    with open(MODE_FILE, encoding='utf-8') as f:
        lines = [x.strip() for x in f.read().splitlines() if x.strip()]
    for i, line in enumerate(lines):
        if i == 0 and '=' not in line:
            mode = line
            continue
        if '=' in line:
            k, v = line.split('=', 1)
            extras[k.strip()] = v.strip()
except Exception as e:
    log('[probe] 读模式文件失败 %r，用默认 gui' % (e,))

# 模式文件里可以追加 QPA 相关的开关，在 import 之前写进进程环境
if extras.get('qpa_dir'):
    os.environ['PYARXQML_QPA_DIR'] = extras['qpa_dir']
if extras.get('platform'):
    os.environ['QT_QPA_PLATFORM'] = extras['platform']
if extras.get('plugin_root'):
    os.environ['PYARXQML_PLUGIN_PATH'] = extras['plugin_root']
if extras.get('qpa_log'):
    os.environ['PYARXQML_QPA_LOG'] = extras['qpa_log']
if extras.get('debug_plugins'):
    # Qt 6 自带：把"检查了哪些插件候选文件"全部打到 stderr / 日志
    os.environ['QT_DEBUG_PLUGINS'] = extras['debug_plugins']


log('')
log('=== probe mode=%s qpa=%r plugin_dir_env=%r' % (
    mode, os.environ.get('QT_QPA_PLATFORM'), os.environ.get('PYARXQML_QPA_DIR')))

import PyArxQml
PyArxQml.set_log(LOG)

t0 = time.perf_counter()
if mode == 'core':
    ok = PyArxQml.ensure_core()
else:
    ok = PyArxQml.ensure()
el = (time.perf_counter() - t0) * 1000
log('[probe] %s() 返回 %s，总耗时 %.1f ms' % ('ensure_core' if mode == 'core' else 'ensure', ok, el))

if mode != 'core':
    log('[probe] stats = %s' % (PyArxQml.stats(),))
