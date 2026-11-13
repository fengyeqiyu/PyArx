# -*- coding: utf-8 -*-
"""PyArxQml 的 Python 便捷封装：一句 show() 就能把 QML 面板挂进 CAD 侧栏。

用法：
    from pyarx_qml import show
    panel = show(r'G:/PyArx/PyArxQml/qml/panel_demo.qml', name='QT-QML')

    import PyArxQml
    PyArxQml.set_callback(lambda op, a, b: '%d + %d = %d' % (a, b, a + b) if op == 'add' else 'ok')
"""
import sys

import PyArxQml

DEFAULT_LOG = r'G:\PyArx\docs\_pyarx_qml.log'


def _init_log(path=None):
    try:
        PyArxQml.set_log(path or DEFAULT_LOG)
    except Exception:
        pass


class QmlPanel(object):
    """一个已挂进侧栏的 QML 面板。"""

    def __init__(self, palette, page, name):
        self.palette = palette
        self.page = page
        self.name = name

    @property
    def hwnd(self):
        return self.page.hwnd

    def push(self, text):
        return PyArxQml.push(text)

    def query(self, prop):
        return PyArxQml.query(prop)

    # ---- Python 控制界面（根对象属性 / 子控件 / QML 函数）----
    def set(self, name, value):
        """写 qml 根对象的属性，如 panel.set('pyResult', 6912)"""
        return PyArxQml.set_prop(name, value)

    def get(self, name):
        """读 qml 根对象的属性"""
        return PyArxQml.get_prop(name)

    def set_named(self, obj_name, prop, value):
        """按 objectName 改子控件属性，如 panel.set_named('pyText','text','x')"""
        return PyArxQml.set_named(obj_name, prop, value)

    def get_named(self, obj_name, prop):
        return PyArxQml.get_named(obj_name, prop)

    def has_item(self, obj_name):
        return PyArxQml.has_item(obj_name)

    def call(self, method, *args):
        """调 qml 根对象上的 JS 函数并取返回值"""
        return PyArxQml.call(method, *args)

    def call_named(self, obj_name, method, *args):
        return PyArxQml.call_named(obj_name, method, *args)

    # ---- 不用手写 objectName：按对象树定位 ----
    def tree(self, max_depth=99, max_nodes=500):
        """导出整棵 QML 对象树，返回 [{'path': '0/2/1', 'type': 'Text', ...}, ...]"""
        return PyArxQml.tree(max_depth, max_nodes)

    def find(self, **criteria):
        """按 type / text / objectName 等条件找控件，返回节点 dict 或 None
        例：panel.find(type='Text', text='等待 Python')
        """
        for node in self.tree() or []:
            ok = True
            for k, v in criteria.items():
                got = node.get(k)
                if k == 'text':
                    if not (got and v in got):
                        ok = False
                        break
                elif got != v:
                    ok = False
                    break
            if ok:
                return node
        return None

    def set_path(self, path, prop, value):
        return PyArxQml.set_path(path, prop, value)

    def get_path(self, path, prop):
        return PyArxQml.get_path(path, prop)

    def call_path(self, path, method, *args):
        return PyArxQml.call_path(path, method, *args)

    # ---- 对齐另两套的通用入口 ----
    def names(self, max_depth=99, max_nodes=2000):
        """全量 {objectName: 类型} 字典，对齐 PyMfcCore.xaml 的 Document.names()"""
        return PyArxQml.names(max_depth, max_nodes)

    def eval(self, expr, obj_name=None):
        """在 QML 上下文里执行 JS 表达式并取回结果，如 panel.eval("pyResult")"""
        if obj_name is None:
            return PyArxQml.eval(expr)
        return PyArxQml.eval(expr, obj_name)

    def stats(self):
        return PyArxQml.stats()

    def grab(self, png_path):
        return PyArxQml.grab(png_path)

    def stop(self):
        return PyArxQml.stop()

    def show(self):
        self.palette.show()


def show(qml_path, name='QT-QML', width=340, height=620, log_path=None, auto_show=True):
    """建侧栏页签 + 把 QML 渲染进去，返回 QmlPanel。"""
    from PyArxCore import Ap

    _init_log(log_path)

    palette = Ap.getPalette(name, width, height)
    page = palette.addPage(name)
    if auto_show:
        palette.show()

    ok = PyArxQml.attach(qml_path, page.hwnd, width, height)
    if not ok:
        raise RuntimeError('PyArxQml.attach 失败，详见日志 %s' % (log_path or DEFAULT_LOG))

    return QmlPanel(palette, page, name)


def attach(qml_path, hwnd, width=340, height=620, log_path=None):
    """已有页窗口的场合：只挂 QML，不建侧栏。"""
    _init_log(log_path)
    return bool(PyArxQml.attach(qml_path, hwnd, width, height))
