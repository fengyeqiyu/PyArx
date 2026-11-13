# -*- coding: utf-8 -*-
"""CAD 侧：把 HTML 侧栏跑起来（PyArxCore.loadPalette → MFCWebView），并把结果写进日志。
日志 G:\\PyArx\\docs\\_palette_web.log 用于确认脚本真的在 CAD 里执行了（COM 注入易静默失败）。
"""
import sys
import traceback

LOG = r'G:\PyArx\docs\_palette_web.log'


def L(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(str(msg) + '\n')


L('--- start ---')
try:
    import PyArxCore
    L('PyArxCore: ' + str(getattr(PyArxCore, '__file__', '?')))

    ap = PyArxCore.Ap
    L('Ap ok: ' + str(ap))

    pal = ap.loadPalette(r'G:\PyArx\X64\Release\palette.html', 'PY-WEB', 340, 560)
    L('loadPalette ok: ' + repr(pal))

    # 关键：脚本结束后页对象不能被回收，否则侧栏内容会跟着消失
    setattr(sys, '_py_web_pal', pal)
    try:
        L('hwnd=' + hex(pal.hwnd))
    except Exception:
        L('hwnd 读取失败')
    L('done')
except Exception:
    L('ERR\n' + traceback.format_exc())
