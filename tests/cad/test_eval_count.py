# -*- coding: utf-8 -*-
"""最小回调测试：让页面 post 一条 cad.count（不跑 Python），看桥是否能回执。"""
import json

from PyArxCore import Ap  # noqa: E402

js = 'window.chrome.webview.postMessage(' + json.dumps({'cmd': 'cad.count'}) + ');'
print('paletteEval ->', Ap.paletteEval('PY-WEB', js))
