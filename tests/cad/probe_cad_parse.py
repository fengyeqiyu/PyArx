# -*- coding: utf-8 -*-
"""在 CAD 里用 PyMfcWebView.pyd 自己的 parseJson 试同一条报文，判断是不是链进去的那份坏了。"""
LOG = r'G:\PyArx\docs\_cad_parse_probe.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


log('=== CAD 内解析探测 ===')
try:
    import PyMfcWebView as wv
    log('PyMfcWebView 导入成功：%r' % (wv,))
    log('parseJson 存在：%s' % hasattr(wv, 'parseJson'))
    try:
        r = wv.parseJson('{"cmd":"cad.count"}')
        log('parseJson 返回 %r' % (r,))
    except Exception as e:
        log('parseJson 抛异常 %r' % (e,))
except Exception as e:
    log('PyMfcWebView 导入失败 %r' % (e,))
log('=== 结束 ===')
