# -*- coding: utf-8 -*-
"""隔离测试阶段 C：页面 post 一条 py.exec（跑 Python + print，落地日志证明执行过）。"""
import json
import time

from PyArxCore import Ap  # noqa: E402

LOG = r'G:\PyArx\docs\_isolate.log'
EXEC_LOG = r'G:\PyArx\docs\_isolate_exec.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('%s %s\n' % (time.strftime('%H:%M:%S'), msg))


open(EXEC_LOG, 'w', encoding='utf-8').close()

code = '\n'.join([
    'import sys',
    'f = open(r"%s", "a", encoding="utf-8")' % EXEC_LOG,
    'f.write("回调执行了，sys.stdout=%s.%s\\n" % (type(sys.stdout).__module__, type(sys.stdout).__name__))',
    'f.flush()',
    'print("页面触发：print 第一行")',
    'print("页面触发：print 第二行")',
    'f.write("print 已调用\\n")',
    'f.close()',
    'result = "页面触发的脚本跑完了"',
])

js = 'window.chrome.webview.postMessage(' + json.dumps(
    {'cmd': 'py.exec', 'code': code, 'args': {'来源': 'paletteEval', '序号': 2}},
    ensure_ascii=False) + ');'

log('阶段 C：py.exec')
log('eval -> %s' % Ap.paletteEval('PY-WEB', js))
log('阶段 C 结束（随后外部等 15 秒）')
