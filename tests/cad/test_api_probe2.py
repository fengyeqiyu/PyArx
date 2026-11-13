# -*- coding: utf-8 -*-
"""看 paletteEval / paletteEmit / getPalette 的签名说明。"""
LOG = r'G:\PyArx\docs\_api_probe2.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


from PyArxCore import Ap  # noqa: E402

for name in ('loadPalette', 'loadPaletteWeb', 'getPalette', 'paletteEmit', 'paletteEval',
             'destroyAllPalettes'):
    fn = getattr(Ap, name, None)
    log('%s: %s' % (name, getattr(fn, '__doc__', None)))
    log('')
log('=== 结束 ===')
