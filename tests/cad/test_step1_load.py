# -*- coding: utf-8 -*-
"""步骤 1：装侧栏，然后让页面 post 一条 cad.count（C++ 路径，不跑 Python）。"""
LOG = r'G:\PyArx\docs\_step.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


import json

from PyArxCore import Ap  # noqa: E402

log('--- 步骤 1：loadPalette ---')
pal = Ap.loadPalette(r'G:\PyArx\X64\Release\palette.html', 'PY-WEB', 340, 560)
log('loadPalette -> %r' % (pal,))

js = 'window.chrome.webview.postMessage(' + json.dumps({'cmd': 'cad.count'}) + ');'
log('cad.count eval -> %s' % Ap.paletteEval('PY-WEB', js))
log('--- 步骤 1 结束 ---')
