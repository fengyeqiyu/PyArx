# -*- coding: utf-8 -*-
"""验证「宿主 → 页面」的 eval 通道：从 CAD 侧调 paletteEval 往页面注入 JS。

页面里 webview_status.html 暴露了 window.__hostPing(v)，收到后把状态表的
「宿主 eval」一行点亮——用它证明 eval() 真的进了页面上下文，而不是只
"发出去了"。注意 eval 只回传"是否执行"（bool），JS 的返回值拿不回来，
要取数据得让页面用 postMessage 回推。
"""
from PyArxCore import Ap

ok = Ap.paletteEval('PY-SELF', "window.__hostPing && window.__hostPing('宿主 eval 通道 OK · 深色方案')")
print('[eval 探针] paletteEval -> %r' % (ok,))
