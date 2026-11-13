# -*- coding: utf-8 -*-
"""分清 eval / emit 两条通道：同一页面上分别试一次，看谁返回 True。"""
import time
from PyArxCore import Ap

for name in ('SUM-DEMO', 'PY-SELF'):
    r1 = Ap.paletteEval(name, "window.__probeRcv ? 1 : 2")
    print('[probe] %-9s eval -> %r' % (name, r1))
    r2 = Ap.paletteEmit(name, '{"cmd":"probe.ping","text":"hello"}')
    print('[probe] %-9s emit -> %r' % (name, r2))
    time.sleep(0.5)

# 用 eval 直接写字，绕开 message 通道，验证文本能改
ok = Ap.paletteEval('SUM-DEMO', "document.getElementById('out').textContent='eval 直写 OK';")
print('[probe] SUM-DEMO eval-write -> %r' % (ok,))
