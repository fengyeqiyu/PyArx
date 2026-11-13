# -*- coding: utf-8 -*-
"""步骤 2：让页面 post 一条 py.exec（跑 Python 并 print），验证回调不炸宿主机。"""
LOG = r'G:\PyArx\docs\_step.log'
EXEC_LOG = r'G:\PyArx\docs\_exec_step.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


import json

from PyArxCore import Ap  # noqa: E402

open(EXEC_LOG, 'w', encoding='utf-8').close()

code = '\n'.join([
    'import sys',
    'f = open(r"%s", "a", encoding="utf-8")' % EXEC_LOG,
    'f.write("py.exec 回调确实执行了\\n")',
    'f.write("sys.stdout=%s.%s\\n" % (type(sys.stdout).__module__, type(sys.stdout).__name__))',
    'print("页面触发：print 第一行")',
    'print("页面触发：print 第二行")',
    'f.write("print 已调用\\n")',
    'f.close()',
    'result = "页面触发的脚本跑完了"',
])

js = 'window.chrome.webview.postMessage(' + json.dumps(
    {'cmd': 'py.exec', 'code': code, 'args': {'来源': 'paletteEval', '序号': 2}},
    ensure_ascii=False) + ');'

log('--- 步骤 2：py.exec ---')
log('eval -> %s' % Ap.paletteEval('PY-WEB', js))
log('--- 步骤 2 结束 ---')
