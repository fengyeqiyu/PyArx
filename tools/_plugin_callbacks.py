import io

# ---------- 1) xaml：把回调直接写在 XAML 里 ----------
p = 'G:/PyArx/plugins/side_panel.xaml'
s = io.open(p, encoding='utf-8').read()
s = s.replace('<Button Name="BtnApply" Grid.Row="2" Grid.Column="1" Text="应用"/>',
              '<Button Name="BtnApply" Grid.Row="2" Grid.Column="1" Text="应用" on_click="on_apply"/>')
s = s.replace('<Button Name="BtnLoad" Grid.Row="2" Grid.Column="2" Text="载入"/>',
              '<Button Name="BtnLoad" Grid.Row="2" Grid.Column="2" Text="载入" on_click="on_load"/>')
s = s.replace('<Button Name="BtnClose" Grid.Row="2" Grid.Column="3" Text="关闭"/>',
              '<Button Name="BtnClose" Grid.Row="2" Grid.Column="3" Text="关闭" on_click="on_close"/>')
s = s.replace('on_click="on_apply"/>', 'on_click="on_apply"/>')
s = s.replace('  加载方（pp_plugin.Composite）',
              '  回调也写在 XAML 里：属性 on_click / on_dblclick / on_change / on_grid 写的是 Python 函数名，\n  加载时按名字在调用方作用域里找函数并绑定，Python 侧不用再 on(...) 挂一遍。\n\n  加载方（pp_plugin.Composite）')
io.open(p, 'w', encoding='utf-8', newline='').write(s)

# ---------- 2) pp_plugin：解析 on_* 属性 + 自动生成/导入 ----------
p = 'G:/PyArx/tests/mfc_xaml/pp_plugin.py'
s = io.open(p, encoding='utf-8').read()

s = s.replace("""import os
import xml.etree.ElementTree as ET

import PyMfcXaml as M""", """import importlib
import inspect
import os
import types
import xml.etree.ElementTree as ET

import PyMfcXaml as M""")

# __init__ 增 scope / 自动事件表
s = s.replace("""    def __init__(self, app, xaml, x, y, w, h, id_base=None):""",
"""    EVENTS = (('on_click', 'click'), ('on_dblclick', 'doubleclick'),
              ('on_change', 'change'), ('on_grid', 'grid'))

    def __init__(self, app, xaml, x, y, w, h, id_base=None, scope=None):""")
s = s.replace("""        self._cb = {}         # (name, event) -> [cb]""",
"""        self._cb = {}         # (name, event) -> [cb]
        self._decl = {}       # name -> {event: handler 名}（XAML 里声明的）
        # 回调函数名去哪里找：显式 scope > app 对象 > 加载方模块的全局
        self.scope = scope if scope is not None else _caller_globals()
        self.app_scope = app""")

# _build 里记录 XAML 声明并自动绑定
s = s.replace("""            self.rects[name] = (ax, ay, int(rw), int(rh))""",
"""            decl = {}
            for attr, ev in self.EVENTS:
                fn = el.get(attr) or el.get(attr.replace('_', '-'))
                if fn:
                    decl[ev] = fn
            if decl:
                self._decl[name] = decl
                for ev in decl:
                    self.on(name, ev, None)

            self.rects[name] = (ax, ay, int(rw), int(rh))""")

# on()：cb 允许为 None（XAML 声明的情况），改成分派器
s = s.replace("""    def on(self, name, event, cb):
        \"\"\"event: click / doubleclick / change / grid。\"\"\"
        self._cb.setdefault((name, event), []).append(cb)
        cid = self.ids.get(name)""",
"""    def on(self, name, event, cb):
        \"\"\"event: click / doubleclick / change / grid。cb=None 表示用 XAML 里声明的函数名。\"\"\"
        if cb is not None:
            self._cb.setdefault((name, event), []).append(cb)
        cid = self.ids.get(name)""")

# 分派器把 _decl 里的函数名解析出来
s = s.replace("""        if event in ('click', 'doubleclick'):
            def route(_id=None, _n=name):
                for f in self._cb[(_n, event)]:
                    f(self, _n, None)""",
"""        if event in ('click', 'doubleclick'):
            def route(_id=None, _n=name, _e=event):
                self._dispatch(_n, _e, None)""")
s = s.replace("""        elif event == 'change':
            def route2(_id=None, text='', _n=name):
                for f in self._cb[(_n, event)]:
                    f(self, _n, text)""",
"""        elif event == 'change':
            def route2(_id=None, text='', _n=name, _e=event):
                self._dispatch(_n, _e, text)""")
s = s.replace("""        elif event == 'grid':
            def route3(action, row, col, text, _n=name):
                for f in self._cb[(_n, event)]:
                    f(self, _n, (action, row, col, text))""",
"""        elif event == 'grid':
            def route3(action, row, col, text, _n=name, _e=event):
                self._dispatch(_n, _e, (action, row, col, text))""")

# 追加解析与分派
s = s.replace("""    # ---------- 改内容：Python 侧只调用语义化方法 ----------""",
"""    # ---------- XAML 声明的事件 → Python 函数 ----------
    def _resolve(self, fname):
        if isinstance(fname, str):
            for scope in (self.scope, getattr(self.app_scope, '__dict__', {})):
                f = scope.get(fname)
                if f is None and hasattr(self.app_scope, fname):
                    f = getattr(self.app_scope, fname)
                if callable(f):
                    return f
            raise NameError('复合控件 %s：XAML 里声明的回调 %s 找不到' % (self.name, fname))
        return fname

    def _invoke(self, fn, name, payload):
        try:
            n = len([p for p in inspect.signature(fn).parameters.values()
                     if p.kind in (p.POSITIONAL_OR_KEYWORD, p.VAR_POSITIONAL)])
        except (TypeError, ValueError):
            n = 2
        if n >= 3:
            return fn(self, name, payload)
        if n == 2:
            return fn(self, name)
        if n == 1:
            return fn(name)
        return fn()

    def _dispatch(self, name, event, payload):
        for fname in self._decl.get(name, {}).get(event, []) and [self._decl[name][event]] or []:
            self._invoke(self._resolve(fname), name, payload)
        for f in self._cb.get((name, event), []):
            self._invoke(f, name, payload)

    # ---------- 改内容：Python 侧只调用语义化方法 ----------""")

# 模块级：调用方作用域 + 自动生成插件模块
s = s.replace("""def available():""",
"""def _caller_globals(depth=3):
    try:
        frame = inspect.currentframe()
        for _ in range(depth):
            frame = frame.f_back
        return frame.f_globals if frame else {}
    except Exception:
        return {}


def import_plugin(kind, app, x, y, w, h, scope=None):
    \"\"\"import 即建控件：插件目录里没有 <kind>.py 就自动生成一个薄壳模块，
    导入它、调用它的 create(app, x, y, w, h)，回调按 XAML 里声明的名字绑定。\"\"\"
    pydir = os.path.dirname(os.path.abspath(__file__))
    if pydir not in os.sys.path:
        os.sys.path.insert(0, pydir)
    modname = 'plugin_' + kind
    path = os.path.join(PLUGIN_DIR, kind + '.py')
    if not os.path.exists(path):
        with open(path, 'w', encoding='utf-8', newline='\\n') as fp:
            fp.write('# 自动生成：%s 复合控件的加载薄壳\\n' % kind)
            fp.write('import pp_plugin\\n\\n\\n')
            fp.write('def create(app, x, y, w, h, scope=None):\\n')
            fp.write("    return pp_plugin.load(app, %r, x, y, w, h, scope=scope)\\n" % kind)
    if modname in os.sys.modules:
        mod = importlib.reload(os.sys.modules[modname])
    else:
        spec = importlib.util.spec_from_file_location(modname, path)
        mod = types.ModuleType(modname)
        mod.__file__ = path
        spec.loader.exec_module(mod)
        os.sys.modules[modname] = mod
    return mod.create(app, x, y, w, h, scope=scope if scope is not None else _caller_globals(2))


def available():""")

s = s.replace("""def load(app, kind, x, y, w, h):
    \"\"\"按复合控件名（= xaml 文件名去扩展名）加载。\"\"\"
    path = os.path.join(PLUGIN_DIR, kind + '.xaml')
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    return Composite(app, path, x, y, w, h)""",
"""def load(app, kind, x, y, w, h, scope=None):
    \"\"\"按复合控件名（= xaml 文件名去扩展名）加载。\"\"\"
    path = os.path.join(PLUGIN_DIR, kind + '.xaml')
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    return Composite(app, path, x, y, w, h, scope=scope)""")

io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('pp_plugin 已支持 XAML 声明回调 + 自动生成插件模块')
