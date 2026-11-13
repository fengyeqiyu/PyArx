# -*- coding: utf-8 -*-
"""隔离测试阶段 B：页面 post 一条 cad.count（C++ 直调路径）。"""
import json
import time

from PyArxCore import Ap  # noqa: E402

LOG = r'G:\PyArx\docs\_isolate.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('%s %s\n' % (time.strftime('%H:%M:%S'), msg))


log('阶段 B：cad.count')
js = 'window.chrome.webview.postMessage(' + json.dumps({'cmd': 'cad.count'}) + ');'
log('cad.count eval -> %s' % Ap.paletteEval('PY-WEB', js))
log('阶段 B 结束（随后外部等 15 秒）')
