# -*- coding: utf-8 -*-
"""最小闭环：Python 算一个加法，把结果回传到侧栏页面里的静态文本。

链路：
    Python（本脚本）算 a + b
      → Ap.paletteEmit('SUM-DEMO', json)      宿主 → 页面（PostWebMessageAsJson）
        → 页面 chrome.webview 'message' 事件
          → document.getElementById('out').textContent = 结果

为什么重复发几次：WebView2 是异步起窗的，页面刚 Navigate 完还没挂好监听时
第一次 PostWebMessageAsJson 会掉进虚空，所以按秒重发，最后一条生效即可。
"""
import json
import time
from PyArxCore import Ap

LOG = r'G:\PyArx\docs\_sum_demo.log'
HTML = r'G:\PyArx\tests\cad\sum_demo.html'
NAME = 'SUM-DEMO'

a, b = 1234, 5678
result = a + b                      # 结果在 Python 侧算出来
text = '%d + %d = %d' % (a, b, result)


def log(msg):
    line = '%s %s\n' % (time.strftime('%H:%M:%S'), msg)
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(line)


log('== 相加测试开始 ==')
pal = Ap.loadPalette(HTML, NAME, 320, 480)
log('loadPalette -> hwnd=%d title=%s' % (int(pal.hwnd), pal.title))

payload = json.dumps({'cmd': 'sum.result', 'text': text}, ensure_ascii=False)
log('Python 算出：%s' % text)

# WebView2 异步建窗：早期消息由 WebViewHost 排队、NavigationCompleted 后补发，
# 这里仍然多发几次，既覆盖"补发前页面还没挂监听"，也便于日志看通道是否稳定。
for i in range(10):
    ok = Ap.paletteEmit(NAME, payload)
    log('paletteEmit #%d -> %r' % (i, ok))
    time.sleep(1)

log('== 相加测试结束 ==')
print('[sum_demo] %s' % text)
