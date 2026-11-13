import io
p = 'G:/PyArx/MfcXamlRuntime/runtime.py'
s = io.open(p, encoding='utf-8').read()

# 1) _resolve：找不到时返回 None（不再抛），并补 __main__ 兜底
old = """    def _resolve(self, fname):
        if isinstance(fname, str):
            for scope in (self.scope, getattr(self.app_scope, '__dict__', {})):
                f = scope.get(fname)
                if f is None and hasattr(self.app_scope, fname):
                    f = getattr(self.app_scope, fname)
                if callable(f):
                    return f
            raise NameError('复合控件 %s：XAML 里声明的回调 %s 找不到' % (self.name, fname))
        return fname"""
new = """    def _resolve(self, fname):
        \"\"\"XAML 里写的函数名 → Python 函数。找不到返回 None（不能抛：
        抛出去会把同一控件上"属性赋值"绑的回调一起挡掉，那才是主要用法）。\"\"\"
        if not isinstance(fname, str):
            return fname
        scopes = [self.scope, getattr(self.app_scope, '__dict__', {})]
        main = sys.modules.get('__main__')
        if main is not None:
            scopes.append(main.__dict__)
        for scope in scopes:
            if not scope:
                continue
            f = scope.get(fname)
            if callable(f):
                return f
        if hasattr(self.app_scope, fname):
            f = getattr(self.app_scope, fname)
            if callable(f):
                return f
        return None"""
assert s.count(old) == 1
s = s.replace(old, new)

# 2) _dispatch：声明解析失败只记一次，继续跑属性绑定的回调
old2 = """    def _dispatch(self, name, event, payload):
        for fname in self._decl.get(name, {}).get(event, []) and [self._decl[name][event]] or []:
            self._invoke(self._resolve(fname), name, payload)
        for f in self._cb.get((name, event), []):
            self._invoke(f, name, payload)"""
new2 = """    def _dispatch(self, name, event, payload):
        fname = self._decl.get(name, {}).get(event)
        if fname:
            fn = self._resolve(fname)
            if fn is not None:
                self._invoke(fn, name, payload)
            elif fname not in self._missing:
                # XAML 声明了但作用域里没这个函数：记一次，不抛、不阻断后面的回调
                self._missing.add(fname)
                print('[MfcXamlRuntime] %s: XAML 声明的回调 %s 未定义，跳过'
                      % (self.name, fname), file=sys.stderr)
        for f in self._cb.get((name, event), []):
            if f is not None:
                self._invoke(f, name, payload)"""
assert s.count(old2) == 1
s = s.replace(old2, new2)

# 3) _missing 集合
s = s.replace("""        self._decl = {}       # name -> {event: handler 名}（XAML 里声明的）""",
"""        self._decl = {}       # name -> {event: handler 名}（XAML 里声明的）
        self._missing = set()  # 已经报过"声明名找不到"的函数名""")

# 4) 作用域取调用方真作用域（原来取到的是本模块内部帧，永远找不到）
s = s.replace("""def _caller_globals(depth=3):""",
"""def _scope_here(depth=1):
    \"\"\"取调用方模块的 globals（脚本以 __main__ 运行也能拿到）。\"\"\"
    try:
        return sys._getframe(depth).f_globals
    except Exception:
        return {}


def _caller_globals(depth=3):""")

s = s.replace("""    comp = Composite(app, path, x, y, w, h, scope=scope)
    return comp""",
"""    comp = Composite(app, path, x, y, w, h, scope=scope if scope is not None else _scope_here(2))
    return comp""")

s = s.replace("""    return Composite(app, path, x, y, w, h, scope=scope)""",
"""    return Composite(app, path, x, y, w, h,
                     scope=scope if scope is not None else _scope_here(2))""")

s = s.replace("""        c = cls._factory[kind](app, x, y, w, h, scope=scope)""",
"""        c = cls._factory[kind](app, x, y, w, h,
                               scope=scope if scope is not None else _scope_here(2))""")

s = s.replace("""import importlib
import importlib.util
import inspect""",
"""import importlib
import importlib.util
import inspect
import sys""")

s = s.replace("""        for f in self._cb.get((name, event), []):
            self._invoke(f, name, payload)

    # ---------- 改内容""",
"""        for f in self._cb.get((name, event), []):
            if f is not None:
                self._invoke(f, name, payload)

    # ---------- 改内容""")

io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('runtime: 声明名解析容错 + 作用域取调用方 globals')
