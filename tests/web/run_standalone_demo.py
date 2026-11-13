# -*- coding: utf-8 -*-
"""纯 Python 解释器里跑 PyMfcWebView.pyd（不依赖 CAD）：

    window = wv.webLoad(页面, 宽, 高, 标题, onCommand=..., onReady=...)
    window.run()

页面上的控件全部是 web，按钮点了就到下面的 onCommand 里，print 的输出打在控制台。
运行：G:\\PyArx\\X64\\Release\\python.exe G:\\PyArx\\tests\\web\\run_standalone_demo.py
"""
import json
import platform
import sys

sys.path.insert(0, r'G:\PyArx\X64\Release')

import PyMfcWebView as wv  # noqa: E402

PAGE = r'G:\PyArx\tests\web\standalone_demo.html'
LOG = r'G:\PyArx\docs\_standalone_py.log'

window = None   # webLoad 返回后才有值；回调里用


def log(text):
    with open(LOG, 'a', encoding='utf-8') as fh:
        fh.write(str(text) + '\n')


def reply(text, ok=True):
    window.emit(json.dumps({'event': 'py.result', 'ok': ok, 'text': text}, ensure_ascii=False))


def onCommand(raw):
    """页面消息回调：返回 True 表示这条已处理。"""
    try:
        data = json.loads(raw)
    except Exception:
        return False
    cmd = data.get('cmd', '')
    args = data.get('args')

    if cmd == 'py.hello':
        # 这就是要验证的：点页面按钮 → Python 里 print 出消息
        print('=== 页面按钮 ① 触发的 Python 代码 ===')
        print('收到命令：py.hello')
        print('页面参数：%r' % (args,))
        print('Python 版本：%s' % sys.version.split()[0])
        log('py.hello args=%r' % (args,))
        reply('Python 已经 print 了 4 行，看控制台；页面参数=%r' % (args,))
        return True

    if cmd == 'py.add':
        a = float(args['a'])
        b = float(args['b'])
        total = a + b
        print('页面要算：%s + %s = %s' % (a, b, total))
        log('py.add %s + %s = %s' % (a, b, total))
        reply('a + b = %s' % total)
        return True

    if cmd == 'py.env':
        text = '%s / %s / Python %s' % (platform.system(), platform.machine(), sys.version.split()[0])
        print('环境信息：' + text)
        log('py.env ' + text)
        reply(text)
        return True

    return False


def onReady(raw):
    print('WebView 就绪回调：%s' % raw)
    log('ready ' + raw)


print('调用 webLoad() ...')
window = wv.webLoad(PAGE, 720, 560, 'PyArx 纯 Python 窗体',
                    onCommand=onCommand, onReady=onReady)
print('webLoad 返回：hwnd=%d，isReady=%s' % (window.hwnd(), window.isReady()))
window.run()
print('窗口已关闭')
