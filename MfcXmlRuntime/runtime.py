# -*- coding: utf-8 -*-
"""MFCXml 运行时框架：XML 组合基础控件 → plugin 目录 → 独立控件对象 + Python 回调。

机制（按约定的三条）：
1. MFCXml 只提供控件库（Button / Label / TextBox / ListCtrl / StaticLine / Panel）；
   复合控件不新增 C++ 类，而是"基础控件 + XML 布局"的组合。
2. 复合控件定义放在 `G:\\PyArx\\plugins\\*.xml`，一个文件 = 一个复合控件，
   根元素 `<XWindow Name="...">` 就是控件类型名，里面的 `x:Name` 是子控件名。
3. 所有子控件的事件统一在本类里转成 Python 回调：`on(name, event, cb)`，
   cb 收到 (composite, name, payload)，调用方不用记 id、不用自己算坐标。

用法：
    sp = Composite(app, r'G:\\PyArx\\plugins\\side_panel.xml', x, y, w, h)
    sp.on('BtnApply', 'click', lambda c, n, p: print('应用被点了'))
    sp.set_text('Title', '共 39 行')
"""

import importlib
import importlib.util
import inspect
import sys
import os
import types
import xml.etree.ElementTree as ET

import PyMfcXml as M

# 插件目录不再写死盘符：优先环境变量，其次本包所在仓库的 plugins\
_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLUGIN_DIR = os.environ.get('PYARX_PLUGIN_DIR') or os.path.join(_ROOT, 'plugins')

# XML 标签 → (宿主建控件的方法, 传参形态)。加一种基础控件只需要往这里补一行。
# text = (id, text, x, y, w, h) / rect = (id, x, y, w, h) / slider = 带量程
_TAG_SPEC = {
    'Label': ('addLabel', 'text'), 'Static': ('addLabel', 'text'),
    'Button': ('addButton', 'text'), 'PushButton': ('addButton', 'text'),
    'TextField': ('addEdit', 'text'), 'Edit': ('addEdit', 'text'),
    'CheckBox': ('addCheckBox', 'text'), 'Radio': ('addRadio', 'text'),
    'Group': ('addGroup', 'text'), 'GroupBox': ('addGroup', 'text'),
    'Toggle': ('addToggle', 'text'), 'Switch': ('addToggle', 'text'),
    'Combo': ('addCombo', 'rect'), 'ComboBox': ('addCombo', 'rect'),
    'Progress': ('addProgress', 'rect'), 'ProgressBar': ('addProgress', 'rect'),
    'Slider': ('addSlider', 'slider'),
    'Tab': ('addTab', 'rect'), 'TabBar': ('addTab', 'rect'),
    'Tree': ('addTree', 'rect'), 'TreeView': ('addTree', 'rect'), 'TreeView': ('addTree', 'rect'),
    'ListCtrl': ('addListCtrl', 'rect'), 'ListBox': ('addListBox', 'rect'),
    'Table': ('addTable', 'rect'), 'GridLayout': ('addGrid', 'rect'),
    'DatePicker': ('addDatePicker', 'rect'), 'Calendar': ('addCalendar', 'rect'),
    'SplitView': ('addSplitter', 'rect'), 'Divider': ('addStaticLine', 'rect'),
    'TextArea': ('addRichText', 'rect'),
    'Image': ('addStaticBitmap', 'rect'), 'Image': ('addStaticBitmap', 'rect'),
    'Pane': ('addPanel', 'rect'), 'ScrollView': ('addScrolledWindow', 'rect'),
}
_TAGS = {k: v[0] for k, v in _TAG_SPEC.items()}     # 兼容按老表引用的地方

# XML 属性 → 事件名。两套写法（on_click 与 Click）都认。
EVENT_ALIASES = {
    'click': 'click', 'on_click': 'click', 'onclick': 'click', 'Click': 'click',
    'doubleclick': 'doubleclick', 'dblclick': 'doubleclick', 'on_dblclick': 'doubleclick',
    'DoubleClick': 'doubleclick',
    'change': 'change', 'textchanged': 'change', 'on_change': 'change', 'TextChanged': 'change',
    'commit': 'commit', 'blur': 'commit', 'lostfocus': 'commit', 'LostFocus': 'commit',
    'select': 'select', 'selectionchanged': 'select', 'on_select': 'select', 'SelectionChanged': 'select',
    'value': 'value', 'valuechanged': 'value', 'ValueChanged': 'value',
    'checked': 'checked', 'checkedchanged': 'checked', 'CheckedChanged': 'checked',
    'dropdown': 'dropdown', 'DropDown': 'dropdown',
    'grid': 'grid', 'on_grid': 'grid',
    # 控件级原始事件（宿主子类化后真派发）
    'mousedown': 'mousedown', 'on_mousedown': 'mousedown', 'MouseDown': 'mousedown',
    'mouseup': 'mouseup', 'on_mouseup': 'mouseup', 'MouseUp': 'mouseup',
    'mousemove': 'mousemove', 'on_mousemove': 'mousemove', 'MouseMove': 'mousemove',
    'keydown': 'keydown', 'on_keydown': 'keydown', 'KeyDown': 'keydown',
    'keyup': 'keyup', 'on_keyup': 'keyup', 'KeyUp': 'keyup',
    'gotfocus': 'gotfocus', 'on_gotfocus': 'gotfocus', 'GotFocus': 'gotfocus',
    'on_lostfocus': 'lostfocus',
}
EVENT_SPEC = (
    (('on_click', 'Click'), 'click'),
    (('on_dblclick', 'DoubleClick'), 'doubleclick'),
    (('on_change', 'TextChanged'), 'change'),
    (('on_commit', 'LostFocus'), 'commit'),
    (('on_select', 'SelectionChanged'), 'select'),
    (('on_value', 'ValueChanged'), 'value'),
    (('on_checked', 'CheckedChanged'), 'checked'),
    (('on_dropdown', 'DropDown'), 'dropdown'),
    (('on_grid', 'CellAction'), 'grid'),
    (('on_mousedown', 'MouseDown'), 'mousedown'),
    (('on_mouseup', 'MouseUp'), 'mouseup'),
    (('on_mousemove', 'MouseMove'), 'mousemove'),
    (('on_keydown', 'KeyDown'), 'keydown'),
    (('on_keyup', 'KeyUp'), 'keyup'),
    (('on_gotfocus', 'GotFocus'), 'gotfocus'),
    (('on_lostfocus', 'LostFocusEvent'), 'lostfocus'),
)

_X = '{http://schemas.microsoft.com/winfx/2006/xml}' 


def _bare_tag(tag):
    """统一命名：<XButton> / <XGrid.RowDefinitions> 去前缀取本意名。"""
    tag = tag.split('}')[-1]
    if '.' in tag:
        tag = tag.rsplit('.', 1)[-1]
    if len(tag) > 1 and tag[0] in 'Xx' and tag[1].isupper():
        tag = tag[1:]
    return tag


def _name_of(el):
    """同一份 XML 里 x:Name 和 Name 都要认：guiload 只认 x:Name，
    复合控件历史上写 Name=，两边各写一种就互相看不见。"""
    for key in (_X + 'Name', 'x:Name', 'Name', 'name', 'id'):
        v = el.get(key)
        if v:
            return v
    return None


# 控件级原始事件：宿主用子类化过程真派发（不再"声明了却不触发"）
CTL_EVENTS = {
    'mousedown': 'mousedown', 'mouseup': 'mouseup', 'mousemove': 'mousemove',
    'keydown': 'keydown', 'keyup': 'keyup',
    'gotfocus': 'gotfocus', 'lostfocus': 'lostfocus',
}

_ID_BASE = 20000        # 复合控件的 id 段，避免和主应用 id 撞
_ID_STEP = 200          # 每个复合控件实例占 200 个 id


class Child(object):
    """复合控件里的一个子控件：`c.BtnApply.click = fn` 直接绑事件。"""

    EVENTS = (('click', 'click'), ('dblclick', 'doubleclick'),
              ('change', 'change'), ('commit', 'commit'),
              ('select', 'select'), ('value', 'value'), ('checked', 'checked'),
              ('dropdown', 'dropdown'), ('grid', 'grid'))

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

    # mybutton.click = fn 这样直接绑；每个事件一个可写属性
    click = property(lambda self: self._comp._cb.get((self._name, 'click'), None),
                     lambda self, fn: self._set_event('click', fn))
    dblclick = property(lambda self: self._comp._cb.get((self._name, 'doubleclick'), None),
                        lambda self, fn: self._set_event('doubleclick', fn))
    change = property(lambda self: self._comp._cb.get((self._name, 'change'), None),
                      lambda self, fn: self._set_event('change', fn))
    grid = property(lambda self: self._comp._cb.get((self._name, 'grid'), None),
                    lambda self, fn: self._set_event('grid', fn))

    def _get(ev):
        return lambda self: self._comp._cb.get((self._name, ev), None)

    def _put(ev):
        return lambda self, fn: self._set_event(ev, fn)

    commit = property(_get('commit'), _put('commit'))
    select = property(_get('select'), _put('select'))
    value = property(_get('value'), _put('value'))
    checked = property(_get('checked'), _put('checked'))
    dropdown = property(_get('dropdown'), _put('dropdown'))
    mousedown = property(_get('mousedown'), _put('mousedown'))
    mouseup = property(_get('mouseup'), _put('mouseup'))
    mousemove = property(_get('mousemove'), _put('mousemove'))
    keydown = property(_get('keydown'), _put('keydown'))
    keyup = property(_get('keyup'), _put('keyup'))
    gotfocus = property(_get('gotfocus'), _put('gotfocus'))
    lostfocus = property(_get('lostfocus'), _put('lostfocus'))

    def __repr__(self):
        return '<Child %s id=%s rect=%s>' % (self._name, self.id, self.rect)


def _host_name(host, snake):
    """宿主方法探测：以 snake_case 为准，同时认旧 camelCase 写法。
    返回真正可调用的名字，没有则 None（认错名字会让事件静默不触发）。"""
    if host is None:
        return None
    if hasattr(host, snake):
        return snake
    head, _, tail = snake.partition('_')
    camel = head + ''.join(p[:1].upper() + p[1:] for p in tail.split('_') if p)
    return camel if hasattr(host, camel) else None


class Composite(object):
    """一个 XML 定义的复合控件实例。"""

    _used = [0]

    EVENTS = (('on_click', 'click'), ('on_dblclick', 'doubleclick'),
              ('on_change', 'change'), ('on_commit', 'commit'), ('on_grid', 'grid'))

    def __init__(self, app, xml, x, y, w, h, id_base=None, scope=None, root=None):
        self.app = app
        self.win = app.win if hasattr(app, 'win') else app
        self.name = os.path.splitext(os.path.basename(xml))[0]
        self.xml = xml
        self.diagnostics = []     # 被静默跳过的东西，加载完一次性报出来
        self._root = root         # 复用调用方已经解析好的树（不重复解析 XML）

        if id_base is None:
            id_base = _ID_BASE + _ID_STEP * Composite._used[0]
            Composite._used[0] += 1
        self.id_base = id_base

        self.rects = {}       # name -> (x, y, w, h) 绝对坐标
        self.ids = {}         # name -> id
        self.kinds = {}       # name -> XML 标签
        self.texts = {}       # name -> Text 属性
        self._cb = {}         # (name, event) -> [cb]
        self._decl = {}       # name -> {event: handler 名}（XML 里声明的）
        self._missing = set()  # 已经报过"声明名找不到"的函数名
        self._missing_evt = set()  # 已经报过"宿主不支持该事件"的事件名
        # 回调函数名去哪里找：显式 scope > app 对象 > 加载方模块的全局
        self.scope = scope if scope is not None else _caller_globals()
        self.app_scope = app

        self._build(x, y, w, h)

    # ---------- 建控件：几何全部来自 xmlLayout ----------
    def _build(self, x, y, w, h):
        rel = M.xml_layout(self.xml, w, h)
        root = self._root if self._root is not None else ET.parse(self.xml).getroot()

        idx = 0
        for el in root.iter():
            name = _name_of(el)
            tag = _bare_tag(el.tag)
            if not name or tag in ('Window', 'Grid', 'RowDefinition', 'ColumnDefinition'):
                continue
            spec = _TAG_SPEC.get(tag)
            if spec is None:
                # 没有这个标签的建法：以前直接 continue，用户只看到控件没出现
                self.diagnostics.append('%s: 标签 <%s> 没有对应的创建方法' % (name, tag))
                continue
            if name not in rel:
                self.diagnostics.append('%s: xmlLayout 没有给出 <%s> 的矩形' % (name, tag))
                continue
            method, shape = spec
            if not hasattr(self.win, method):
                self.diagnostics.append('%s: 宿主缺少 %s()，<%s> 未创建'
                                        % (name, method, tag))

            rx, ry, rw, rh = rel[name]
            cid = self.id_base + idx
            idx += 1
            text = el.get('Text') or el.get('Content') or el.get('Header') or ''
            ax, ay = x + int(rx), y + int(ry)

            if shape == 'text':
                getattr(self.win, method)(cid, text, ax, ay, int(rw), int(rh))
            elif shape == 'slider':
                lo = int(el.get('Minimum') or 0)
                hi = int(el.get('Maximum') or 100)
                getattr(self.win, method)(cid, lo, hi, ax, ay, int(rw), int(rh))
            else:
                getattr(self.win, method)(cid, ax, ay, int(rw), int(rh))

            self._bind_declared(el, name)

            self.rects[name] = (ax, ay, int(rw), int(rh))
            setattr(self, name, Child(self, name))
            self.ids[name] = cid
            self.kinds[name] = tag
            self.texts[name] = text

        if self.diagnostics:
            print('[MfcXmlRuntime] %s：%d 个控件未创建'
                  % (self.name, len(self.diagnostics)), file=sys.stderr)
            for d in self.diagnostics[:10]:
                print('  - ' + d, file=sys.stderr)

    def _bind_declared(self, el, name):
        """把 XML 里声明的 on_xxx="fn" / Click="fn" 记下来并挂上路由。"""
        for attrs, ev in EVENT_SPEC:
            fn = None
            for a in attrs:
                fn = el.get(a) or el.get(a.replace('_', '-'))
                if fn:
                    break
            if fn:
                self._decl.setdefault(name, {})[ev] = fn
                self.on(name, ev, None)

    # ---------- 事件：一律回 Python ----------
    # 宿主方法名统一 snake_case（Python 封面的对外口径）；同时兼容旧 camel 写法，
    # 谁在就调谁。认错名字会让 child.click 静默不触发，所以两条都试。
    _HOST_METHOD = {
        'click': ('on_click', 'onClick'),
        'doubleclick': ('on_double_click', 'onDoubleClick'),
        'change': ('on_change', 'onChange'),          # 实时文本变化（不是失焦）
        'select': ('on_select', 'onSelect'),
        'value': ('on_value', 'onValue'),
        'checked': ('on_checked', 'onChecked'),
        'dropdown': ('on_drop_down', 'onDropDown'),
    }

    def on(self, name, event, cb):
        """event: click / doubleclick / change / commit / select / value /
        checked / dropdown / grid；也接受 XML 属性名的写法（Click、TextChanged…）。"""
        event = EVENT_ALIASES.get(event, event)
        if cb is not None:
            self._cb.setdefault((name, event), []).append(cb)
        cid = self.ids.get(name)
        if cid is None:
            return self

        if event == 'grid':
            route = lambda *a, **k: self._dispatch(name, 'grid', a)
            m = _host_name(self.win, 'on_grid_action_for')
            if m:                                       # 按控件过滤，多表并存不串台
                getattr(self.win, m)(cid, route)
            else:
                m = _host_name(self.win, 'on_grid_action')
                if m:
                    getattr(self.win, m)(route)
            return self

        if event == 'commit':                       # 失焦才提交
            m = _host_name(self.win, 'on_blur')
            if m:
                getattr(self.win, m)(cid, lambda *a, **k: self._dispatch(name, 'commit', a[1] if len(a) > 1 else None))
            return self

        ctl = CTL_EVENTS.get(event)
        if ctl is not None:
            m = _host_name(self.win, 'on_control_event')
            if not m:
                if event not in self._missing_evt:
                    self._missing_evt.add(event)
                    print('[MfcXmlRuntime] 事件 %s 在当前宿主上不可用，已忽略（%s）'
                          % (event, self.name), file=sys.stderr)
                return self
            getattr(self.win, m)(cid, ctl, lambda *a, _n=name, _e=event: self._dispatch(_n, _e, None))
            return self

        candidates = self._HOST_METHOD.get(event)
        if isinstance(candidates, str):        # 老代码可能还在用字符串表
            candidates = (candidates,)
        method = None
        if candidates:
            for cand in candidates:
                if hasattr(self.win, cand):
                    method = cand
                    break
        if method is None:
            if event not in self._missing_evt:
                self._missing_evt.add(event)
                print('[MfcXmlRuntime] 事件 %s 在当前宿主上不可用，已忽略（%s）'
                      % (event, self.name), file=sys.stderr)
            return self

        def route(*a, _n=name, _e=event, **k):
            payload = None
            if len(a) > 1:
                payload = a[1]
            elif a and not isinstance(a[0], int):
                payload = a[0]
            if _e == 'change' and isinstance(payload, str):
                self.texts[_n] = payload
            self._dispatch(_n, _e, payload)
        getattr(self.win, method)(cid, route)
        return self

    def __getattr__(self, name):
        ids = self.__dict__.get('ids') or {}
        if name in ids:
            return Child(self, name)
        raise AttributeError(name)

    # ---------- XML 声明的事件 → Python 函数 ----------
    def _resolve(self, fname):
        """XML 里写的函数名 → Python 函数。找不到返回 None（不能抛：
        抛出去会把同一控件上"属性赋值"绑的回调一起挡掉，那才是主要用法）。"""
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
        return None

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
        fname = self._decl.get(name, {}).get(event)
        if fname:
            fn = self._resolve(fname)
            if fn is not None:
                self._invoke(fn, name, payload)
            elif fname not in self._missing:
                # XML 声明了但作用域里没这个函数：记一次，不抛、不阻断后面的回调
                self._missing.add(fname)
                print('[MfcXmlRuntime] %s: XML 声明的回调 %s 未定义，跳过'
                      % (self.name, fname), file=sys.stderr)
        for f in self._cb.get((name, event), []):
            if f is not None:
                self._invoke(f, name, payload)

    # ---------- 改内容：Python 侧只调用语义化方法 ----------
    def set_text(self, name, text):
        cid = self.ids.get(name)
        if cid is not None:
            self.win.set_text(cid, text)

    def set_rows(self, name, rows):
        """rows: [ [第1格, 第2格, ...], ... ]  一次灌满。"""
        cid = self.ids.get(name)
        if cid is None:
            return
        self.win.set_table_rows(cid, '\n'.join('\t'.join(r) for r in rows))

    def move(self, x, y, w, h):
        """窗口尺寸变了：整块重排，仍然交给 XML 算矩形。"""
        rel = M.xml_layout(self.xml, w, h)
        batch = []
        for name, (rx, ry, rw, rh) in rel.items():
            cid = self.ids.get(name)
            if cid is not None:
                ax, ay = x + int(rx), y + int(ry)
                batch.append((cid, ax, ay, int(rw), int(rh)))
                self.rects[name] = (ax, ay, int(rw), int(rh))
        if batch:
            self.win.move_many(batch)      # 一次搬完，不逐个 move（40 个逐个 move 会掉帧）

    def destroy(self):
        for cid in self.ids.values():
            try:
                self.win.removeControl(cid)
            except Exception:
                pass
        self.ids.clear()
        self.rects.clear()


class Runtime(object):
    """运行机制框架：控件类型注册表 + 生命周期。

    - `register(kind, factory)`：把 plugin 目录里的 XML 组合登记成一种控件类型；
      factory 缺省就是 `load(app, kind, ...)`，也可以换成 C++ 侧同名控件（接口不变）。
    - `create(kind, app, x, y, w, h)`：按登记的类型建实例。
    - `list()` / `unload(kind)`：列出 / 注销。
    """
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
            if not os.path.exists(os.path.join(PLUGIN_DIR, kind + '.xml')):
                raise KeyError('未登记的复合控件：%s' % kind)
            cls.register(kind)
        c = cls._factory[kind](app, x, y, w, h,
                               scope=scope if scope is not None else _scope_here(2))
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


def _scope_here(depth=1):
    """取调用方模块的 globals（脚本以 __main__ 运行也能拿到）。"""
    try:
        return sys._getframe(depth).f_globals
    except Exception:
        return {}


def _caller_globals(depth=3):
    try:
        frame = inspect.currentframe()
        for _ in range(depth):
            frame = frame.f_back
        return frame.f_globals if frame else {}
    except Exception:
        return {}


def import_plugin(kind, app, x, y, w, h, scope=None):
    """import 即建控件：插件目录里没有 <kind>.py 就自动生成一个薄壳模块，
    导入它、调用它的 create(app, x, y, w, h)，回调按 XML 里声明的名字绑定。"""
    pydir = os.path.dirname(os.path.abspath(__file__))
    if pydir not in os.sys.path:
        os.sys.path.insert(0, pydir)
    modname = 'plugin_' + kind
    path = os.path.join(PLUGIN_DIR, kind + '.py')
    if not os.path.exists(path):
        with open(path, 'w', encoding='utf-8', newline='\n') as fp:
            fp.write('# 自动生成：%s 复合控件的加载薄壳\n' % kind)
            fp.write('import pp_plugin\n\n\n')
            fp.write('def create(app, x, y, w, h, scope=None):\n')
            fp.write("    return pp_plugin.load(app, %r, x, y, w, h, scope=scope)\n" % kind)
    if modname in os.sys.modules and os.path.exists(path):
        mod = importlib.reload(os.sys.modules[modname])
    else:
        spec = importlib.util.spec_from_file_location(modname, path)
        mod = types.ModuleType(modname)
        mod.__file__ = path
        spec.loader.exec_module(mod)
        os.sys.modules[modname] = mod
    return mod.create(app, x, y, w, h, scope=scope if scope is not None else _caller_globals(2))


def xmlload(path, w=None, h=None, app=None, scope=None, x=0, y=0):
    """`mybutton = xmlload(path)`，然后 `mybutton.BtnApply.click = fn` 绑事件。

    只给 path 时按 XML 根元素的 Width/Height 建（此时还没进任何窗口，仅取矩形与对象图）；
    给了 w/h 就按这个外框算矩形；给了 app 才真的在窗口里创建控件。
    """
    if not os.path.isabs(path):
        path = os.path.join(PLUGIN_DIR, path)
    root = ET.parse(path).getroot()          # 只解析一次：尺寸与控件树共用同一棵树
    if w is None or h is None:
        w = int(root.get('Width') or 480)
        h = int(root.get('Height') or 320)
    comp = Composite(app, path, x, y, w, h, scope=scope if scope is not None else _scope_here(2),
                     root=root)
    return comp


def available():
    """列出 plugin 目录里可用的复合控件。"""
    if not os.path.isdir(PLUGIN_DIR):
        return []
    return sorted(f for f in os.listdir(PLUGIN_DIR) if f.lower().endswith('.xml'))


def load(app, kind, x, y, w, h, scope=None):
    """按复合控件名（= xml 文件名去扩展名）加载。"""
    path = os.path.join(PLUGIN_DIR, kind + '.xml')
    if not os.path.exists(path):
        raise FileNotFoundError(path)
    return Composite(app, path, x, y, w, h,
                     scope=scope if scope is not None else _scope_here(2))
