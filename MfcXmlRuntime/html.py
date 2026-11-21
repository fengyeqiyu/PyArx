# -*- coding: utf-8 -*-
"""HTML 风格的界面创建层（MfcXml 的"便利入口"）。

一行出控件：
    from MfcXmlRuntime import html
    ui = html('<div><label id="t">标题</label><button id="ok" onclick="on_ok">确定</button></div>',
              app=win, w=320)

它只做"改写"，不做第二套渲染：
    小写标签 -> 基础控件标签    文本内容 -> Text 属性     id -> x:Name
    onclick  -> Click           没写 width 的块级控件 -> 撑满内容宽
改写结果交给 M.guiload_text，走的仍是 XmlRuntime 解析 / 布局 / 控件注册表。
坐标（Left/Top）由 XmlRuntime 的流式布局给出，尺寸（W/H）由本层按 HTML 规则算好。

规则（都是确定的，没有魔法）：
* 容器：div/section/article/form/nav/header/footer/main/ul/li/ol/fieldset/fragment（竖排）
        row/hbox/toolbar/inline（横排）
* 控件：button label p span h1-h4 input textarea select checkbox radio img hr table
        progress slider switch list tree grid scroll link toggle icon tabs group dialog
* 属性：id、width、height、padding、spacing/gap、value/text；事件 onclick/onchange/
        onselect/oncheck/ondblclick/onkeydown/onkeyup/onfocus/onblur/onmousedown...
* class 只当记号保留，不解释（样式表还没接）
"""
import sys
import xml.etree.ElementTree as ET

import PyMfcXml as M

from .runtime import Composite, Child

__all__ = ['html', 'Ui', 'TAGS', 'BLOCK', 'ROW', 'DEFAULT_HEIGHT', 'DEFAULT_WIDTH']

# 标签 -> 基础控件
TAGS = {
    'button': 'XButton', 'label': 'XLabel', 'p': 'XLabel', 'span': 'XLabel',
    'strong': 'XLabel', 'b': 'XLabel', 'em': 'XLabel',
    'h1': 'XLabel', 'h2': 'XLabel', 'h3': 'XLabel', 'h4': 'XLabel',
    'input': 'XTextField', 'textarea': 'XTextArea', 'select': 'XComboBox',
    'checkbox': 'XCheckBox', 'radio': 'XRadioButton', 'img': 'XImage',
    'hr': 'XDivider', 'table': 'XTableView', 'progress': 'XProgressBar',
    'slider': 'XSlider', 'switch': 'XSwitch', 'list': 'XListBox',
    'tree': 'XTreeView', 'grid': 'XGridLayout', 'scroll': 'XScrollView',
    'link': 'XHyperlink', 'toggle': 'XToggleButton', 'icon': 'XIconButton',
    'tabs': 'XTabBar', 'group': 'XGroupBox', 'dialog': 'XDialog',
}

# 容器（竖排）与行内容器（横排）
BLOCK = {'div', 'section', 'article', 'body', 'form', 'nav', 'header', 'footer',
         'main', 'ul', 'li', 'ol', 'fieldset', 'fragment'}
ROW = {'row', 'hbox', 'toolbar', 'inline'}

# 默认高度：没写 height 时用；写了就以写的为准
DEFAULT_HEIGHT = {
    'XButton': 32, 'XLabel': 22, 'XTextField': 30, 'XTextArea': 88,
    'XComboBox': 28, 'XCheckBox': 24, 'XRadioButton': 24, 'XSwitch': 26,
    'XImage': 96, 'XDivider': 1, 'XProgressBar': 18, 'XSlider': 26,
    'XListBox': 160, 'XTreeView': 160, 'XTableView': 180, 'XGridLayout': 160,
    'XScrollView': 160, 'XHyperlink': 22, 'XToggleButton': 26,
    'XIconButton': 32, 'XTabBar': 32, 'XGroupBox': 120, 'XDialog': 140,
}

# 横排时的自然宽度（块级里不写 width 就是撑满，横排里撑满语义不成立，用自然宽）
DEFAULT_WIDTH = {
    'XButton': 96, 'XLabel': 0, 'XTextField': 180, 'XComboBox': 140,
    'XCheckBox': 120, 'XRadioButton': 100, 'XSwitch': 60, 'XDivider': 60,
    'XImage': 96, 'XSlider': 140, 'XProgressBar': 120, 'XHyperlink': 90,
}

# 事件属性 -> MfcXml 事件属性名（guiload 的事件解释器认这些名字）
EVENTS = {
    'onclick': 'Click', 'ondblclick': 'DoubleClick',
    'onchange': 'TextChanged', 'oninput': 'TextChanged',
    'onselect': 'SelectionChanged', 'oncheck': 'CheckedChanged',
    'ondropdown': 'DropDown', 'onwheel': 'Scroll',
    'onmousedown': 'MouseDown', 'onmouseup': 'MouseUp', 'onmousemove': 'MouseMove',
    'onkeydown': 'KeyDown', 'onkeyup': 'KeyUp',
    'onfocus': 'GotFocus', 'onblur': 'LostFocus',
}

# 普通属性小写名 -> MfcXml 属性名
ATTRS = {
    'width': 'Width', 'height': 'Height', 'left': 'Left', 'top': 'Top',
    'value': 'Text', 'text': 'Text', 'content': 'Text', 'title': 'Text',
    'tooltip': 'ToolTip', 'min': 'Minimum', 'max': 'Maximum',
    'checked': 'Checked', 'placeholder': 'Placeholder', 'src': 'Source',
}

_SPACING = 8
_ID_SEQ = [20000]        # html() 每次取一段 id，多次创建不会互相踩


# ---------------------------------------------------------------- 结构节点
class _Node(object):
    __slots__ = ('tag', 'xml_tag', 'name', 'attrs', 'text', 'children',
                 'cont', 'horizontal', 'spacing', 'pad', 'w', 'h')

    def __init__(self, tag):
        self.tag = tag
        self.xml_tag = None
        self.name = None
        self.attrs = []          # [(属性名, 值)]，不含 Width/Height
        self.text = ''
        self.children = []
        self.cont = False
        self.horizontal = False
        self.spacing = _SPACING
        self.pad = (0, 0, 0, 0)  # 左、上、右、下
        self.w = 0               # 0 = 撑满父内容宽（横排里表示按自然宽）
        self.h = 0

    def inner_w(self):
        return max(1, self.w - self.pad[0] - self.pad[2])


def _thickness(v, default=(0, 0, 0, 0)):
    """Padding：12 / 12,8 / 左,上,右,下（与 XmlRuntime 的 ParseThickness 同规则）。"""
    if not v:
        return default
    try:
        parts = [int(p) for p in str(v).replace(' ', '').split(',') if p]
    except ValueError:
        return default
    if len(parts) == 1:
        return (parts[0],) * 4
    if len(parts) == 2:
        return (parts[1], parts[0], parts[1], parts[0])
    if len(parts) == 3:
        return (parts[1], parts[2], parts[1], parts[0])
    return tuple(parts[:4])


class _Builder(object):
    def __init__(self):
        self.n = 0
        self.used = set()

    def _name(self, el):
        raw = el.get('id') or el.get('name')
        if raw:
            name = raw
        else:
            self.n += 1
            name = 'el%d' % self.n
        while name in self.used:
            self.n += 1
            name = '%s_%d' % (name, self.n)
        self.used.add(name)
        return name

    def parse(self, el):
        tag = el.tag.lower().split('}')[-1]
        if tag in BLOCK or tag in ROW:
            return self._parse_container(el, horizontal=(tag in ROW))
        xml_tag = TAGS.get(tag)
        if xml_tag is None:
            raise ValueError(
                'html(): 不认识的标签 <%s>\n  容器：%s\n  控件：%s'
                % (tag, ' '.join(sorted(BLOCK | ROW)), ' '.join(sorted(TAGS))))
        return self._parse_leaf(el, xml_tag)

    def _parse_container(self, el, horizontal=False):
        node = _Node(el.tag)
        node.xml_tag = 'XStackPanel'
        node.cont = True
        node.horizontal = horizontal
        node.name = None          # 容器不建原生控件，也不需要名字（不占自动 id 的号）
        node.spacing = int(el.get('spacing') or el.get('gap') or _SPACING)
        node.pad = _thickness(el.get('padding'))
        node.w = int(el.get('width') or 0)
        node.h = int(el.get('height') or 0)
        node.attrs = _extra_attrs(el, skip=('padding', 'spacing', 'gap', 'orientation'))
        for child in list(el):
            node.children.append(self.parse(child))
        return node

    def _parse_leaf(self, el, xml_tag):
        node = _Node(el.tag)
        node.xml_tag = xml_tag
        node.name = self._name(el)
        node.h = int(el.get('height') or 0) or DEFAULT_HEIGHT.get(xml_tag, 24)
        node.w = int(el.get('width') or 0)

        # 叶子的文本：连内联子标签里的字一起收（<label>共<b>3</b>项</label> -> "共3项"）
        node.text = ' '.join(''.join(el.itertext()).split())
        node.attrs = _extra_attrs(el, skip=('width', 'height'))
        return node

    # ---- 第一趟：宽度（自上而下）
    def measure_width(self, node, avail):
        """avail = 父容器内容宽；返回本节点实际宽度（0 表示"由父填满"）。"""
        if node.cont:
            node.w = node.w or avail
            for child in node.children:
                if node.horizontal:
                    if not child.w:
                        child.w = DEFAULT_WIDTH.get(child.xml_tag, 96) if not child.cont else 96
                    self.measure_width(child, child.w or avail)
                else:
                    child.w = child.w or node.inner_w()
                    self.measure_width(child, child.w)
            return node.w
        return node.w

    # ---- 第二趟：高度（自下而上）
    def measure_height(self, node):
        if not node.cont:
            return node.h
        if node.horizontal:
            body = max([self.measure_height(c) for c in node.children] or [0])
            gap = 0                        # 横排：高度取最高子项，不吃 Spacing
        else:
            body = sum(self.measure_height(c) for c in node.children)
            gap = node.spacing * max(0, len(node.children) - 1)
        node.h = node.h or (body + gap + node.pad[1] + node.pad[3])
        return node.h

    # ---- 序列化
    def emit(self, node):
        a = ['Width="%d"' % node.w]
        if node.name:
            a.insert(0, 'Name="%s"' % node.name)
        if node.cont:
            a.append('Height="%d"' % node.h)
            a.append('Spacing="%d"' % node.spacing)
            a.append('Padding="%d,%d,%d,%d"' % node.pad)
            if node.horizontal:
                a.append('Orientation="Horizontal"')
        else:
            a.append('Height="%d"' % node.h)
            if node.text:
                a.append('Text="%s"' % _esc(node.text))
        a.extend('%s="%s"' % (k, _esc(v)) for k, v in node.attrs)
        if node.cont:
            return '<%s %s>%s</%s>' % (node.xml_tag, ' '.join(a),
                                       ''.join(self.emit(c) for c in node.children),
                                       node.xml_tag)
        return '<%s %s/>' % (node.xml_tag, ' '.join(a))


def _esc(s):
    return (s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
             .replace('"', '&quot;'))


def _extra_attrs(el, skip=()):
    out = []
    for k, v in el.attrib.items():
        low = k.lower()
        if low in ('id', 'name', 'class') or low in skip:
            continue
        if low in EVENTS:
            out.append((EVENTS[low], v))
            continue
        out.append((ATTRS.get(low, k), v))
    return out


# ---------------------------------------------------------------- 结果对象
class Ui(Composite):
    """html() 的返回值：ui.Btn.click = fn、ui.Btn.rect、ui.xml 都照旧可用。"""

    def __init__(self, host, xml_text, ids, w, h, scope=None):
        self.prebuilt = dict(ids or {})
        super(Ui, self).__init__(host, 'inline.xml', 0, 0, w, h, scope=scope)
        self.xml = xml_text          # Composite.__init__ 会把 xml 设成文件名，这里放回最终 XML

    # 控件已经由 guiload_text 建好，这里只登记对象图，不重复创建
    def _build(self, x, y, w, h):
        for name, cid in self.prebuilt.items():
            self.ids[name] = cid
            setattr(self, name, Child(self, name))
        self.refresh_rects()

    def refresh_rects(self):
        """矩形取自真实控件（宿主给的客户区尺寸），不是 Python 算的影子值。"""
        if getattr(self, 'win', None) is None:
            return self.rects
        for name, cid in self.ids.items():
            try:
                cw, ch = self.win.get_size(cid)
                self.rects[name] = (0, 0, int(cw), int(ch))
            except Exception:
                self.rects[name] = None
        return self.rects


# ---------------------------------------------------------------- 入口
def html(source, app=None, w=0, h=0, scope=None, target=None):
    """一行创建界面（字符串入口，不落盘）。

    source  HTML 风格字符串；以 < 开头、根是完整 XML 时也接受
    app     宿主（Window / Panel / 带 .win 的对象）；不给就只算结构不建控件
    w/h     外框尺寸；不给按内容推
    返回 Ui：ui.el1 / ui.某id、ui.rects（真实尺寸）、ui.xml（最终 XML）、ui.ids
    """
    if not isinstance(source, str):
        raise TypeError('html(): source 必须是字符串')
    body = source.strip()
    if not body.startswith('<'):
        raise ValueError('html(): source 必须以 < 开头')

    frag = ET.fromstring('<fragment>%s</fragment>' % body)
    kids = list(frag)
    if not kids:
        raise ValueError('html(): 至少要有一个元素')

    builder = _Builder()
    if len(kids) == 1:
        node = builder.parse(kids[0])
    else:
        node = builder.parse(frag)

    width = node.w or int(w) or 320
    node.w = width
    builder.measure_width(node, width)
    if not node.cont and not node.w:
        node.w = width                     # 光秃秃一个控件当根：撑满窗口
    height = int(h) or builder.measure_height(node)
    node.h = height

    xml_text = ('<?xml version="1.0" encoding="utf-8"?>'
                '<XWindow Width="%d" Height="%d">%s</XWindow>'
                % (node.w, node.h, builder.emit(node)))

    names = {}
    host = None
    if app is not None or target is not None:
        host = target or app
        if hasattr(host, 'win') and host.win is not None:
            host = host.win
        if scope is None:
            # onclick="fn" 里的 fn 按调用方作用域找（C++ 的 CallerGlobals 拿到的
            # 是本模块这一帧，不是用户脚本，所以这里显式补一帧）
            try:
                scope = sys._getframe(1).f_globals
            except Exception:
                scope = None
        first_id = _ID_SEQ[0]
        _ID_SEQ[0] += max(64, len(builder.used) + 8)
        M.guiload_text(xml_text, target=host, context=scope, names=names,
                       first_id=first_id)

    return Ui(host, xml_text, names, node.w, node.h, scope=scope)
