# -*- coding: utf-8 -*-
"""宿主 → 页面：用 paletteEval 把值推进状态页，验证 eval 通道。"""
import time

LOG = r'G:\PyArx\docs\_mwv_status.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('%s %s\n' % (time.strftime('%H:%M:%S'), msg))


from PyArxCore import Ap  # noqa: E402

ok = Ap.paletteEval('PY-SELF', "window.__hostPing && window.__hostPing('CAD 侧押入 ' + new Date().toLocaleTimeString())")
log('paletteEval -> %r' % (ok,))
