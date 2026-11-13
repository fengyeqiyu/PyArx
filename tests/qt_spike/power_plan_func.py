# -*- coding: utf-8 -*-
"""配电干线图 QML 功能验证：折叠、选中表单、搜索、纵向布局、JSON/SVG 导出、PNG 落盘"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import sys
import time

os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\PyArxQml')

QML = r'G:/PyArx/PyArxQml/qml/power_plan/PowerPlan.qml'
PNG = r'G:\PyArx\docs\_pyarx_qml_powerplan.png'
SVG = r'G:\PyArx\docs\_pyarx_qml_powerplan.svg'
LOG = r'G:\PyArx\docs\_powerplan_func.log'

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32
LR = ctypes.c_ssize_t
WNDPROC = ctypes.WINFUNCTYPE(LR, wt.HWND, wt.UINT, ctypes.c_size_t, LR)
user32.DefWindowProcW.restype = LR
user32.DefWindowProcW.argtypes = [wt.HWND, wt.UINT, ctypes.c_size_t, LR]
proc = WNDPROC(lambda h, m, w, l: user32.DefWindowProcW(h, m, w, l))


class WC(ctypes.Structure):
    _fields_ = [('cbSize', ctypes.c_uint), ('style', ctypes.c_uint),
                ('lpfnWndProc', ctypes.c_void_p), ('cbClsExtra', ctypes.c_int),
                ('cbWndExtra', ctypes.c_int), ('hInstance', wt.HANDLE),
                ('hIcon', wt.HANDLE), ('hCursor', wt.HANDLE),
                ('hbrBackground', wt.HANDLE), ('lpszMenuName', wt.LPCWSTR),
                ('lpszClassName', wt.LPCWSTR), ('hIconSm', wt.HANDLE)]


hinst = kernel32.GetModuleHandleW(None)
wc = WC()
wc.cbSize = ctypes.sizeof(wc)
wc.lpfnWndProc = ctypes.cast(proc, ctypes.c_void_p)
wc.hInstance = hinst
wc.lpszClassName = 'PPFuncHost'
user32.RegisterClassExW(ctypes.byref(wc))
hwnd = user32.CreateWindowExW(0, 'PPFuncHost', '配电干线图', 0x00CF0000, 100, 100,
                              1100, 760, None, None, hinst, None)
user32.ShowWindow(hwnd, 5)


def log(msg):
    print(msg)
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(msg + '\n')


def settle(seconds=0.6):
    t0 = time.time()
    while time.time() - t0 < seconds:
        PyArxQml.pump()
        time.sleep(0.008)


import PyArxQml

open(LOG, 'w', encoding='utf-8').close()
log('attach = %s' % PyArxQml.attach(QML, hwnd, 1100, 760))
settle(1.0)
log('info = %s' % PyArxQml.call('info'))

def ev(expr):
    return PyArxQml.eval(expr)

# 1) 折叠 AT1：可见节点应从 39 降到 33（AT1 下 6 个子孙）
log('折叠前 visible = %r' % ev('layout.nodes.length'))
ev("(function(){ toggleCollapse('AT1'); return 1; })()")
settle(0.2)
log('折叠 AT1 后 visible = %r（期望 33）' % ev('layout.nodes.length'))

# 2) 展开
ev("(function(){ toggleCollapse('AT1'); return 1; })()")
settle(0.2)
log('展开后 visible = %r（期望 39）' % ev('layout.nodes.length'))

# 3) 选中终端节点 → 表单
ev("(function(){ selectNode('AT1-1-1'); return 1; })()")
settle(0.3)
log('选中后 selected=%r formVisible=%r fCode=%r' % (ev('selected'), ev('formVisible'), ev('fCode')))
log('表单回填 neCode=%r neName=%r neUp=%r' % (
    PyArxQml.get_named('neCode', 'text'), PyArxQml.get_named('neName', 'text'),
    PyArxQml.get_named('neUp', 'text')))

# 4) 搜索
ev("(function(){ doSearch('AT1-2-3'); return 1; })()")
settle(0.4)
log('搜索 pulse=%r selected=%r' % (ev('pulse'), ev('selected')))

# 5) 纵向布局
ev("(function(){ root_dir = 'v'; return 1; })()" if False else "(function(){ dir='v'; rebuild(); fitView(); return 1; })()")
settle(0.4)
log('纵向 dir=%r 画布 %r×%r zoom=%r' % (ev('dir'), ev('layout.width'), ev('layout.height'), ev('Math.round(k*100)')))
ev("(function(){ dir='h'; rebuild(); fitView(); return 1; })()")
settle(0.3)

# 6) 导出 JSON / SVG 文本
js = PyArxQml.call('exportJsonText')
log('JSON 长度 %d，前 80 字：%s' % (len(js), js[:80]))
d = json.loads(js)
log('JSON 校验：nodes=%d layoutNodes=%d links=%d crossings=%d' % (
    len(d['nodes']), len(d['layout']['nodes']), len(d['layout']['links']), d['checks']['crossings']))
svg = PyArxQml.call('exportSvgText')
log('SVG 长度 %d，含 <svg>=%r 含 titleBlock=%r' % (
    len(svg), ('<svg' in svg), ('图名' in svg)))
with open(SVG, 'w', encoding='utf-8') as f:
    f.write(svg)

# 7) PNG 落盘（QML 侧 grabToImage）
if os.path.exists(PNG):
    os.remove(PNG)
ev("(function(){ exportPngTo('%s'); return 1; })()" % PNG.replace('\\', '/'))
settle(1.2)
log('PNG 存在 = %r 大小 = %r' % (os.path.exists(PNG), os.path.getsize(PNG) if os.path.exists(PNG) else 0))

# 8) 整窗抓图（给用户看）
for _ in range(20):
    PyArxQml.pump()
PyArxQml.grab(PNG.replace('.png', '_ui.png'))
log('整窗抓图 %s' % PNG.replace('.png', '_ui.png'))
log('结束 info = %s' % PyArxQml.call('info'))
