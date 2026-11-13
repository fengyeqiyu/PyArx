# -*- coding: utf-8 -*-
"""CAD 进程内验证：Python 算完结果写回 QML 界面"""
import sys

sys.path.insert(0, r'G:\PyArx\PyArxQml')

LOG = r'G:\PyArx\docs\_pyarx_qml_cad_bridge.log'


def log(msg):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write('[cad] %s\n' % msg)


log('=== 开始（CAD 进程内）')
from pyarx_qml import show

panel = show(r'G:/PyArx/PyArxQml/qml/panel_demo.qml', name='QT-QML',
             width=340, height=620, log_path=LOG)
log('面板已挂上 hwnd=0x%X' % panel.hwnd)

# Python 侧算，结果直接写进 QML 属性（Text 绑定了它，界面立刻变）
a, b = 1234, 5678
panel.set('pyResult', str(a + b))
panel.set('pyTip', 'CAD 进程内 Python 算完写回 QML')
panel.set('pyColor', '#ffd166')
log('set_prop 完成，回读 pyResult=%r' % panel.get('pyResult'))

# 按 objectName 直接改子控件
log('set_named -> %s' % panel.set_named('pyText', 'text', '%d + %d = %d' % (a, b, a + b)))

# 调 QML 里的函数拿返回值
log('call flash -> %r' % panel.call('flash', 'CAD 里 Python 调 QML 函数'))

# 新增接口：names() / eval()，与 PyMfcCore.xaml 的 Document.names() 对齐
log('names -> %r' % (panel.names(),))
log('eval("pyResult") -> %r' % (panel.eval('pyResult'),))
log('eval("2 + 3 * 4") -> %r' % (panel.eval('2 + 3 * 4'),))
log('eval 赋值 pyColor -> %r' % (panel.eval('pyColor = "#FF8800"'),))
log('eval 后回读 pyColor=%r' % panel.get('pyColor'))
log('eval 数组 -> %r' % (panel.eval('[1,2,3].map(function(v){return v*v})'),))

import PyArxQml
log('diag -> %r' % (PyArxQml.diag(),))
for _ in range(80):
    PyArxQml.pump()

panel.grab(r'G:/PyArx/docs/_pyarx_qml_cad_bridge.png')
log('抓图完成')
