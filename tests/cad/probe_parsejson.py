# -*- coding: utf-8 -*-
"""在裸 Python 里直接喂 PyMfcWebView.parseJson，用和页面同样的报文，二分找崩的构造。"""
import json
import sys

import PyMfcWebView as wv

CASES = {}

CASES['最小'] = '{"cmd":"py.exec"}'
CASES['带换行'] = json.dumps({'cmd': 'py.exec', 'code': 'a=1\nb=2'})
CASES['带中文'] = json.dumps({'cmd': 'py.exec', 'code': 'print("中文")'}, ensure_ascii=False)
CASES['带反斜杠'] = json.dumps({'cmd': 'py.exec', 'code': 'open(r"G:\\PyArx\\docs\\x.log")'})
CASES['带嵌套args'] = json.dumps({'cmd': 'py.exec', 'code': 'a=1', 'args': {'来源': '页', '序号': 2}},
                               ensure_ascii=False)
CASES['完整报文'] = json.dumps({
    'cmd': 'py.exec',
    'code': '\n'.join([
        'import sys',
        'f = open(r"G:\\PyArx\\docs\\_selftest_exec.log", "a", encoding="utf-8")',
        'f.write("自检页触发的 py.exec 执行了\\n")',
        'f.write("sys.stdout=%s.%s\\n" % (type(sys.stdout).__module__, type(sys.stdout).__name__))',
        'print("页面自检：这一行来自 Python 的 print")',
        'f.close()',
        'result = "自检完成：脚本在 CAD 里跑完了"',
    ]),
}, ensure_ascii=False)

for name, text in CASES.items():
    print('--- %s len=%d ---' % (name, len(text)))
    try:
        r = wv.parseJson(text)
        print('ok', (r or '')[:60] if isinstance(r, str) else type(r))
    except Exception as e:
        print('异常 %r' % (e,))
