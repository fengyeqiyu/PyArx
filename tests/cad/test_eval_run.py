# -*- coding: utf-8 -*-
"""不用鼠标点：在页面里 eval 一段 JS，让它自己 post 一条 py.exec 给桥。

JSON 用 json.dumps 生成（上一次手写引号导致 JSON 坏掉，桥解析失败）。
脚本内部再写一份落地日志，这样即使页面回执没显示也能拿到证据。
"""
import json

LOG = r'G:\PyArx\docs\_eval_run.log'
PY_LOG = r'G:\PyArx\docs\_eval_exec.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


open(PY_LOG, 'w', encoding='utf-8').close()

from PyArxCore import Ap  # noqa: E402

code = (
    'import sys\n'
    'f = open(r"%s", "a", encoding="utf-8")\n'
    'f.write("py.exec 回调确实执行了\\n")\n'
    'f.write("sys.stdout=%s.%s\\n" %% (type(sys.stdout).__module__, type(sys.stdout).__name__))\n'
    'f.flush()\n'
    'print("页面触发：print 第一行")\n'
    'print("页面触发：print 第二行")\n'
    'f.write("print 已调用\\n")\n'
    'f.close()\n'
    'result = "页面触发的脚本跑完了"\n'
) % (PY_LOG, '%', '%')

js = 'window.chrome.webview.postMessage(' + json.dumps(
    {'cmd': 'py.exec', 'code': code, 'args': {'来源': 'paletteEval 触发', '序号': 1}},
    ensure_ascii=False) + ');'

log('步骤 1：paletteEval 发出 py.exec（JSON 合法）')
log(str(js)[:200])
ok = Ap.paletteEval('PY-WEB', js)
log('步骤 2：paletteEval 返回 %s' % ok)
log('=== 结束：等消息回调 ===')
