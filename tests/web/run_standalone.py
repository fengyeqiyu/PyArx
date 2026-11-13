# -*- coding: utf-8 -*-
"""裸解释器验证：PyMfcWebView.pyd 不依赖 CAD，直接在建窗 + 加载页面 + 页面回调 Python。"""
import json
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')

import PyMfcWebView as wv  # noqa: E402

PAGE = r'G:\PyArx\tests\web\pywebview_demo.html'

window = wv.WebWindow(760, 560, 'PyMfcWebView 独立窗体', PAGE)
print('hwnd=%d ready=%s' % (window.hwnd(), window.isReady()), flush=True)


def onCommand(text):
    try:
        data = json.loads(text)
    except Exception:
        return False
    cmd = data.get('cmd', '')
    if cmd == 'py.ping':
        args = data.get('args') or {}
        window.emit(json.dumps({'event': 'py.result', 'ok': True,
                                'text': 'Python 收到：' + json.dumps(args, ensure_ascii=False)},
                               ensure_ascii=False))
        return True
    if cmd == 'py.resultReceived':
        print('页面回执已到：' + data.get('text', ''), flush=True)
        return True
    return False


window.setCommandHandler(onCommand)
window.center()
window.show()

for i in range(150):
    window.runFor(100)
    if window.isReady():
        print('ready after %dms' % ((i + 1) * 100), flush=True)
        break

window.runFor(60000)
print('closing', flush=True)
window.close()

