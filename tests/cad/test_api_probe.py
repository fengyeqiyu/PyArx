# -*- coding: utf-8 -*-
"""摸清 Ap.Palette 暴露的接口，便于后续不需要鼠标就能触发页面回调。"""
LOG = r'G:\PyArx\docs\_api_probe.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


from PyArxCore import Ap  # noqa: E402

log('Ap 成员：')
for n in dir(Ap):
    if not n.startswith('_'):
        log('  %s = %r' % (n, getattr(Ap, n, None)))

log('Palette 方法：')
cls = getattr(Ap, 'Palette', None)
for n in dir(cls):
    if not n.startswith('_'):
        log('  %s' % n)
log('=== 探测结束 ===')
