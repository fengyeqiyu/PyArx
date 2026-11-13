import io
p = 'G:/PyArx/tests/mfc_xaml/pp_plugin.py'
s = io.open(p, encoding='utf-8').read()

# ChildProxy：让 c.BtnApply.click = fn 这种写法成立
s = s.replace("""class Composite(object):""",
"""class Child(object):
    \"\"\"复合控件里的一个子控件：`c.BtnApply.click = fn` 直接绑事件。\"\"\"

    EVENTS = (('click', 'click'), ('dblclick', 'doubleclick'),
              ('change', 'change'), ('grid', 'grid'))

    def __init__(self, comp, name):
        object.__setattr__(self, '_comp', comp)
        object.__setattr__(self, '_name', name)

    @property
    def name(self):
        return self._name

    @property
    def id(self):
        return self._comp.ids.get(self._name)

    @property
    def rect(self):
        return self._comp.rects.get(self._name)

    def _set_event(self, ev, fn):
        if fn is None:
            self._comp._cb.pop((self._name, ev), None)
        else:
            self._comp.on(self._name, ev, fn)
        return fn

    def __repr__(self):
        return '<Child %s id=%s rect=%s>' % (self._name, self.id, self.rect)


class Composite(object):""")

# _build 里给每个子控件挂 Child 属性
s = s.replace("""            self.rects[name] = (ax, ay, int(rw), int(rh))""",
"""            self.rects[name] = (ax, ay, int(rw), int(rh))
            setattr(self, name, Child(self, name))""")

# 模块级 xamlload
s = s.replace("""def available():""",
"""def xamlload(path, w=None, h=None, app=None, scope=None, x=0, y=0):
    \"\"\"`mybutton = xamlload(path)`，然后 `mybutton.BtnApply.click = fn` 绑事件。

    只给 path 时按 XAML 根元素的 Width/Height 建（此时还没进任何窗口，仅取矩形与对象图）；
    给了 w/h 就按这个外框算矩形；给了 app 才真的在窗口里创建控件。
    \"\"\"
    if not os.path.isabs(path):
        path = os.path.join(PLUGIN_DIR, path)
    if w is None or h is None:
        root = ET.parse(path).getroot()
        w = int(root.get('Width') or 480)
        h = int(root.get('Height') or 320)
    comp = Composite(app, path, x, y, w, h, scope=scope)
    return comp


def available():""")

# Composite 支持 __getattr__：没被 setattr 覆盖的名字也能拿到 Child
s = s.replace("""    # ---------- XAML 声明的事件 → Python 函数 ----------""",
"""    def __getattr__(self, name):
        ids = self.__dict__.get('ids') or {}
        if name in ids:
            return Child(self, name)
        raise AttributeError(name)

    # ---------- XAML 声明的事件 → Python 函数 ----------""")
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('pp_plugin: xamlload + c.Name.click = fn 已加')
