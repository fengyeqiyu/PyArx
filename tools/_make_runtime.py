import io
p = 'G:/PyArx/MfcXamlRuntime/runtime.py'
s = io.open(p, encoding='utf-8').read()

# 头部说明改成运行时框架
s = s.replace('"""复合控件加载器：XAML 组合基础控件 → plugin 目录 → 封成独立控件。',
              '"""MFCXaml 运行时框架：XAML 组合基础控件 → plugin 目录 → 独立控件对象 + Python 回调。')

# 追加：注册表 / 生命周期
s = s.replace("""def _caller_globals(depth=3):""",
"""class Runtime(object):
    \"\"\"运行机制框架：控件类型注册表 + 生命周期。

    - `register(kind, factory)`：把 plugin 目录里的 XAML 组合登记成一种控件类型；
      factory 缺省就是 `load(app, kind, ...)`，也可以换成 C++ 侧同名控件（接口不变）。
    - `create(kind, app, x, y, w, h)`：按登记的类型建实例。
    - `list()` / `unload(kind)`：列出 / 注销。
    \"\"\"
    _factory = {}
    _inst = {}

    @classmethod
    def register(cls, kind, factory=None):
        cls._factory[kind] = factory or (lambda app, x, y, w, h, scope=None:
                                         load(app, kind, x, y, w, h, scope=scope))
        return kind

    @classmethod
    def create(cls, kind, app, x, y, w, h, scope=None):
        if kind not in cls._factory:
            if not os.path.exists(os.path.join(PLUGIN_DIR, kind + '.xaml')):
                raise KeyError('未登记的复合控件：%s' % kind)
            cls.register(kind)
        c = cls._factory[kind](app, x, y, w, h, scope=scope)
        cls._inst[id(c)] = c
        return c

    @classmethod
    def list(cls):
        kinds = set(cls._factory)
        for f in available():
            kinds.add(os.path.splitext(f)[0])
        return sorted(kinds)

    @classmethod
    def unload(cls, kind):
        cls._factory.pop(kind, None)


def _caller_globals(depth=3):""")

io.open(p, 'w', encoding='utf-8', newline='').write(s)

# __init__：包入口
init_py = '''# -*- coding: utf-8 -*-
"""MfcXamlRuntime：复合控件的运行机制框架。

    from MfcXamlRuntime import xamlload, Runtime

    c = xamlload(r'G:\\PyArx\\plugins\\side_panel.xaml', app=app, x=10, y=10, w=640, h=380)
    c.BtnApply.click = myclass.on_click
    c.Table.grid = myclass.on_grid
"""
from .runtime import (Composite, Child, Runtime, available, import_plugin,
                      load, xamlload, PLUGIN_DIR)

__all__ = ['Composite', 'Child', 'Runtime', 'available', 'import_plugin',
           'load', 'xamlload', 'PLUGIN_DIR']
'''
io.open('G:/PyArx/MfcXamlRuntime/__init__.py', 'w', encoding='utf-8', newline='').write(init_py)

# 测试目录里的 pp_plugin 变成兼容转发
shim = '''# -*- coding: utf-8 -*-
"""兼容转发：真正的运行机制在 G:\\PyArx\\MfcXamlRuntime。"""
import os
import sys

_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
                      if False else os.path.abspath(__file__))
for _ in range(2):
    _ROOT = os.path.dirname(_ROOT)
if _ROOT not in sys.path:
    sys.path.insert(0, _ROOT)

from MfcXamlRuntime import *          # noqa: F401,F403
from MfcXamlRuntime import Composite, Child, Runtime, xamlload, load, import_plugin, available  # noqa: F401
'''
io.open('G:/PyArx/tests/mfc_xaml/pp_plugin.py', 'w', encoding='utf-8', newline='').write(shim)
print('运行机制框架已落到 MfcXamlRuntime，tests/pp_plugin 变成兼容转发')
