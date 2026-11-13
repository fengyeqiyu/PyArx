# -*- coding: utf-8 -*-
"""裸 python.exe 里跑 PyMfcWebView：建窗 → 加载 HTML → 收页面命令 → 回推结果。

用法（用 3.14 那个解释器，pyd 是对着 3.14 编的）：
    "D:\\Program Files\\Python314\\python.exe" G:\\PyArx\\tests\\web\\test_pymfcwebview.py
日志：G:\\PyArx\\docs\\_pywebview.log（页面点按钮后同时更新，用来确认真的是页面回调进了 Python）
"""
import json
import os
import sys
import time
import traceback

OUT = r'G:\PyArx\X64\Release'
LOG = r'G:\PyArx\docs\_pywebview.log'
HTML = r'G:\PyArx\tests\web\pywebview_demo.html'

sys.path.insert(0, OUT)


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


with open(LOG, 'w', encoding='utf-8') as f:
    f.write('')

try:
    import PyMfcWebView as wv

    log('模块 OK: ' + str(wv))

    win = wv.WebWindow(900, 640, 'PyArx 独立窗体')
    log('窗口句柄: ' + str(win.hwnd()))

    def onCommand(text):
        log('页面消息: ' + text)
        try:
            data = json.loads(text)
        except Exception:
            return True

        cmd = data.get('cmd')
        if cmd == 'py.ping':
            args = data.get('args') or {}
            # 换个说法回推，证明 JSON 里的中文和嵌套参数都原样到了 Python
            payload = {
                'event': 'py.result',
                'ok': True,
                'text': 'Python 收到 ping：seq=%s，text=%s' % (args.get('seq'), args.get('text')),
            }
            win.emit(json.dumps(payload, ensure_ascii=False))
            return True

        return False   # 其它命令交给 WebShell 内置路由（shell.info / window.close …）

    def onReady(text):
        log('就绪: ' + text)

    win.setCommandHandler(onCommand)
    win.setReadyHandler(onReady)
    win.loadFile(HTML)
    win.show()
    win.center()

    # 等 WebView 初始化，然后把窗口矩形打到日志里，供注入点击用
    for _ in range(60):
        win.runFor(100)
        if win.isReady():
            break
    log('ready=%s' % win.isReady())
    log('hwnd=%s' % win.hwnd())

    # 自己跑一段时间等外部注入点击；时长可由命令行给（默认 45 秒）
    keep = int(sys.argv[1]) if len(sys.argv) > 1 else 45
    win.runFor(keep * 1000)
    log('窗口关闭: %s' % win.isClosed())
    win.destroy()
    log('done')
except Exception:
    log('ERR\n' + traceback.format_exc())
    raise
