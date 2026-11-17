# -*- coding: utf-8 -*-
"""ui —— UI 核心（模块 1）：Ribbon 与工具条的自绘渲染

为什么自绘：
  · 宿主没有可用的 <XToolBar>（guiload 不认这个标签，整条被静默丢掉）
  · <XButton> 只能显示文字，没有图标位，排两列就像一排板砖
  · XPowerCanvas 已经能把任意 SVG 画出来，还能把鼠标事件派发给 Python

所以功能区/工具条 = 一块 XPowerCanvas + 一份 SVG + 一张命中表。
SVG 坐标和控件像素 1:1（setPlanView 1.0/0/0），鼠标坐标直接查表。

配色与尺寸统一走 token，别在函数里另造颜色。
"""
FONT = "Microsoft YaHei, Segoe UI, sans-serif"

# ---- 配色 token（深色主题） ----
C_BG = "#1E2124"        # 应用底色
C_CHROME = "#23272C"    # 页签条 / 工具条底
C_PANEL = "#2B3036"     # 分组面板
C_BTN = "#2F353B"       # 按钮常态
C_BTN_HOT = "#353D45"   # 悬停
C_BTN_DOWN = "#262C33"  # 按下
C_SEP = "#3A4048"       # 分隔线
C_TXT = "#E6EDF3"       # 主文字
C_TXT_DIM = "#9AA4AE"   # 次文字
C_TXT_MUT = "#7D8590"   # 弱文字（组标题）
C_ACCENT = "#4C9AFF"    # 强调（选中指示 / 开启态）

TAB_W = 78
TAB_H = 28
PANEL_TOP = 32
BTN_W = 48
BTN_H = 44
ICON = 24
GAP = 8


# ---------------------------------------------------------------- 基础绘制
def _r(x, y, w, h, fill, stroke=None, rx=0):
    s = ' stroke="%s" stroke-width="1"' % stroke if stroke else ''
    return ('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="%.1f" '
            'fill="%s"%s/>' % (x, y, w, h, rx, fill, s))


def _t(x, y, text, size, color, anchor="middle", bold=False, opacity=None):
    w = ' font-weight="600"' if bold else ''
    o = ' opacity="%.2f"' % opacity if opacity is not None else ''
    return ('<text x="%.1f" y="%.1f" font-family="%s" font-size="%.1f" '
            'fill="%s" text-anchor="%s"%s%s>%s</text>'
            % (x, y, FONT, size, color, anchor, w, o, _esc(text)))


def _esc(s):
    return (str(s).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def _icon(kind, cx, cy, color, size=24):
    """24×24 本地坐标的线框图标，按 size 缩放并居中到 (cx, cy)。"""
    st = 'stroke="%s" stroke-width="1.7" fill="none" stroke-linecap="round"' % color
    g = []

    def L(x1, y1, x2, y2):
        g.append('<line x1="%s" y1="%s" x2="%s" y2="%s" %s/>' % (x1, y1, x2, y2, st))

    def P(d, fill="none"):
        g.append('<path d="%s" %s fill="%s"/>' % (d, st, fill))

    def A(cx0, cy0, r0, fill="none"):
        g.append('<circle cx="%s" cy="%s" r="%s" %s fill="%s"/>' % (cx0, cy0, r0, st, fill))

    def R(x, y, w, h):
        g.append('<rect x="%s" y="%s" width="%s" height="%s" %s/>' % (x, y, w, h, st))

    if kind == "line":
        L(4, 19, 20, 5)
        L(4, 19, 9, 19)
    elif kind == "pline":
        P("M4 19 L9 10 L14 15 L20 5")
        A(4, 19, 1.4, color)
        A(20, 5, 1.4, color)
    elif kind == "circle":
        A(12, 12, 8)
        A(12, 12, 1.1, color)
    elif kind == "arc":
        P("M5 17 A 9 9 0 0 1 19 9")
        A(5, 17, 1.3, color)
    elif kind == "rect":
        R(4, 7, 16, 11)
    elif kind == "text":
        L(5, 6, 19, 6)
        L(12, 6, 12, 19)
        L(8, 19, 16, 19)
    elif kind == "hatch":
        R(4, 6, 16, 12)
        for i in range(4):
            L(6 + i * 3, 16, 12 + i * 3, 8)
    elif kind == "dim":
        L(4, 8, 4, 17)
        L(20, 8, 20, 17)
        L(4, 13, 20, 13)
        P("M7 11 L4 13 L7 15")
        P("M17 11 L20 13 L17 15")
    elif kind == "erase":
        L(6, 6, 18, 18)
        L(18, 6, 6, 18)
    elif kind == "move":
        L(12, 4, 12, 20)
        L(4, 12, 20, 12)
        P("M9 7 L12 4 L15 7")
        P("M9 17 L12 20 L15 17")
        P("M7 9 L4 12 L7 15")
        P("M17 9 L20 12 L17 15")
    elif kind == "copy":
        R(4, 4, 12, 12)
        R(9, 9, 11, 11)
    elif kind == "rotate":
        P("M6 14 A 8 8 0 0 1 18 9")
        P("M15 6 L18 9 L15 12")
        A(12, 12, 1.2, color)
    elif kind == "fit":
        R(4, 6, 16, 12)
        L(9, 10, 15, 10)
        L(9, 14, 15, 14)
        L(9, 10, 9, 14)
        L(15, 10, 15, 14)
    elif kind == "zin":
        A(11, 11, 6)
        L(15, 15, 20, 20)
        L(8, 11, 14, 11)
        L(11, 8, 11, 14)
    elif kind == "zout":
        A(11, 11, 6)
        L(15, 15, 20, 20)
        L(8, 11, 14, 11)
    elif kind == "new":
        P("M6 4 L14 4 L19 9 L19 20 L6 20 Z")
        L(14, 4, 14, 9)
        L(14, 9, 19, 9)
    elif kind == "save":
        R(4, 5, 16, 15)
        R(8, 12, 8, 8)
        L(8, 8, 15, 8)
    elif kind == "export":
        P("M12 16 L12 5")
        P("M8 8 L12 4 L16 8")
        R(5, 15, 14, 5)
    elif kind == "grid":
        R(4, 5, 16, 15)
        L(9, 5, 9, 20)
        L(14, 5, 14, 20)
        L(4, 11, 20, 11)
    elif kind == "layer":
        P("M4 8 L12 4 L20 8 L12 12 Z")
        P("M4 13 L12 17 L20 13")
    elif kind == "undo":
        P("M8 8 L4 12 L8 16")
        P("M4 12 L16 12 A 5 5 0 0 1 16 20")
    elif kind == "redo":
        P("M16 8 L20 12 L16 16")
        P("M20 12 L8 12 A 5 5 0 0 0 8 20")
    elif kind == "select":
        P("M5 5 L5 17 L9 13 L12 20 L15 18 L12 12 L19 12 Z")
    elif kind == "ortho":
        L(6, 6, 18, 6)
        L(6, 6, 6, 18)
        L(9, 15, 15, 9)
    elif kind == "snap":
        L(12, 4, 12, 20)
        L(4, 12, 20, 12)
        A(12, 12, 3)
    else:
        A(12, 12, 6)

    return ('<g transform="translate(%.1f,%.1f) scale(%.3f) translate(-12,-12)">%s</g>'
            % (cx, cy, size / 24.0, "".join(g)))


def _svg(w, h, body, bg=None):
    return ('<?xml version="1.0" encoding="utf-8"?>'
            '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" '
            'width="%d" height="%d"><rect x="0" y="0" width="%d" height="%d" '
            'fill="%s"/>%s</svg>' % (w, h, w, h, w, h, bg or C_BG, body))


# ---------------------------------------------------------------- Ribbon 数据
# (分组名, [(动作, 显示文字, 图标)])
DRAW = [
    ("绘图", [("LINE", "直线", "line"), ("PLINE", "多段线", "pline"),
             ("CIRCLE", "圆", "circle"), ("ARC", "圆弧", "arc"),
             ("RECT", "矩形", "rect")]),
    ("填充与注释", [("HATCH", "填充", "hatch"), ("TEXT", "文字", "text"),
                  ("DIM", "标注", "dim")]),
]
MODIFY = [
    ("修改", [("MOVE", "移动", "move"), ("COPY", "复制", "copy"),
             ("ROTATE", "旋转", "rotate"), ("ERASE", "删除", "erase")]),
    ("选择", [("SELECT", "选择", "select")]),
]
VIEW = [
    ("缩放", [("ZOOM_E", "全图", "fit"), ("ZOOM_IN", "放大", "zin"),
             ("ZOOM_OUT", "缩小", "zout")]),
    ("显示", [("GRID", "栅格", "grid"), ("PANMODE", "平移模式", "move")]),
]
LAYER = [
    ("图层", [("LAYER_NEW", "新建", "layer"), ("LAYER_OFF", "开关", "layer")]),
]
FILE = [
    ("文件", [("SAVE", "保存", "save"), ("EXPORT", "导出", "export")]),
]

TABS = [("DRAW", "常用", DRAW), ("MODIFY", "修改", MODIFY), ("VIEW", "视图", VIEW),
        ("LAYER", "图层", LAYER), ("FILE", "文件", FILE)]


def ribbon(w, h, tab_key="DRAW", hover=None, down=None):
    """生成一份 Ribbon SVG，同时返回命中表 [(x,y,w,h,action)]。"""
    body = [_r(0, 0, w, TAB_H + 4, C_CHROME),
            _r(0, TAB_H + 3, w, 1, C_SEP)]
    hits = []

    # 页签条
    x = 10.0
    body.append(_t(x, 19, "PyArx CAD", 11.5, C_ACCENT, "start", True))
    x += 62
    for key, label, _groups in TABS:
        active = (key == tab_key)
        tw = TAB_W - 6
        body.append(_t(x + tw / 2.0, 20, label, 11.5,
                       C_TXT if active else C_TXT_DIM, bold=active))
        if active:
            body.append(_r(x + 6, TAB_H - 2, tw - 12, 2, C_ACCENT))
        hits.append((x, 0, tw, TAB_H, "TAB:" + key))
        x += TAB_W

    groups = dict(((k, g) for k, _l, g in TABS)).get(tab_key, DRAW)

    # 分组面板
    px = 8.0
    ph = h - PANEL_TOP - 4
    for gname, btns in groups:
        gw = len(btns) * (BTN_W + 4) + 8
        body.append(_r(px, PANEL_TOP, gw, ph, C_PANEL, C_SEP, 4))
        bx = px + 4
        for action, label, icon in btns:
            hot = (hover == action)
            pressed = (down == action)
            fill = C_BTN_DOWN if pressed else (C_BTN_HOT if hot else "none")
            if fill != "none":
                body.append(_r(bx, PANEL_TOP + 5, BTN_W, BTN_H, fill, rx=4))
            ic = "#FFFFFF" if hot else "#C9D1D9"
            body.append(_icon(icon, bx + BTN_W / 2.0, PANEL_TOP + 5 + 22, ic, ICON))
            body.append(_t(bx + BTN_W / 2.0, PANEL_TOP + 5 + BTN_H - 6, label,
                           10.5, "#FFFFFF" if hot else C_TXT))
            hits.append((bx, PANEL_TOP + 5, BTN_W, BTN_H, action))
            bx += BTN_W + 4
        body.append(_t(px + gw / 2.0, PANEL_TOP + ph - 5, gname, 9.5, C_TXT_MUT))
        px += gw + GAP

    return _svg(w, h, "".join(body)), hits


# ---------------------------------------------------------------- 左工具条
# 竖向图标条：只有图标，悬停才浮出一块高亮，一组之间用细线隔开。
TOOLS = [
    ("LINE", "直线", "line"), ("PLINE", "多段线", "pline"), ("CIRCLE", "圆", "circle"),
    ("ARC", "圆弧", "arc"), ("RECT", "矩形", "rect"), ("HATCH", "填充", "hatch"),
    ("TEXT", "文字", "text"), ("DIM", "标注", "dim"),
    "-",
    ("MOVE", "移动", "move"), ("COPY", "复制", "copy"), ("ROTATE", "旋转", "rotate"),
    ("ERASE", "删除", "erase"),
    "-",
    ("SELECT", "选择", "select"), ("FIT", "缩放全图", "fit"),
]

CELL = 40          # 单格高度（4px 倍数）
IC = 24            # 图标尺寸


def toolbar(w, h, hover=None, down=None):
    """竖向图标条：一格一个工具，无边框，悬停高亮。"""
    body = [_r(0, 0, w, h, C_CHROME), _r(w - 1, 0, 1, h, C_SEP)]
    hits = []
    y = 6.0
    for item in TOOLS:
        if item == "-":
            body.append(_r(6, y + 4, w - 12, 1, C_SEP))
            y += 10
            continue
        action, label, icon = item
        hot = (hover == action)
        if hot:
            body.append(_r(3, y + 1, w - 6, CELL - 2,
                           C_BTN_DOWN if down == action else C_BTN_HOT, rx=3))
        body.append(_icon(icon, w / 2.0, y + CELL / 2.0,
                          "#FFFFFF" if hot else "#C9D1D9", IC))
        hits.append((2, y, w - 4, CELL - 2, action))
        y += CELL
    return _svg(w, h, "".join(body)), hits


# ---------------------------------------------------------------- 状态栏
def status_bar(w, h, chips, rights, hover=None):
    """chips: [(文字, 动作key 或 None, 是否开启)]；rights: [文字] 右对齐依次排。"""
    body = [_r(0, 0, w, h, C_CHROME), _r(0, 0, w, 1, C_SEP)]
    hits = []
    x = 4.0
    for text, key, active in chips:
        tw = 14.0 + len(text) * 7.4
        hot = (key is not None and hover == key)
        body.append(_r(x, 3, tw, h - 6, C_BTN_HOT if hot else C_BTN, C_SEP, 3))
        body.append(_t(x + tw / 2.0, h / 2.0 + 4.5, text, 10.5,
                       C_ACCENT if active else C_TXT))
        if key:
            hits.append((x, 2, tw, h - 4, key))
        x += tw + 6

    rx = w - 8.0
    for text in reversed(rights):
        if not text:
            continue
        tw = 14.0 + len(text) * 7.4
        rx -= tw
        body.append(_t(rx + tw / 2.0, h / 2.0 + 4.5, text, 10.5, C_TXT_DIM))
        rx -= 8.0
    return _svg(w, h, "".join(body)), hits


def hit(hits, x, y):
    for hx, hy, hw, hh, action in hits:
        if hx <= x < hx + hw and hy <= y < hy + hh:
            return action
    return None


# ---------------------------------------------------------------- 右栏属性面板
PROP_TITLE_H = 26
PROP_ROW_H = 24


def props_panel(w, h, title, rows, panel_bg=C_PANEL, hits_out=None):
    """属性面板：顶部标题 + 分组（组标题深色条 + 标签左/值右行）。

    rows 两种形态：
      · 扁平 [(label, value[, field]), ...]（兼容旧视图面板）
      · 分组 [(group_title, [(label, value[, field]), ...]), ...]
    行元组第三项 field 为可编辑字段键；hits_out 非空时追加 (x,y,w,h,field) 命中表。
    """
    body = [_r(0, 0, w, h, panel_bg)]
    y = 0.0
    if title:
        body.append(_t(10, y + 18, title, 11.5, C_TXT, "start", True))
        y += PROP_TITLE_H

    def _is_groups(rs):
        return rs and isinstance(rs[0], tuple) and len(rs[0]) == 2 \
            and isinstance(rs[0][1], list)

    groups = rows if _is_groups(rows) else [("", rows)]
    for group_title, grows in groups:
        if group_title:
            body.append(_r(0, y, w, PROP_ROW_H, C_CHROME))
            body.append(_t(10, y + 16, group_title, 10.5, C_TXT, "start", True))
            y += PROP_ROW_H
        for row in grows:
            label, value = row[0], row[1]
            field = row[2] if len(row) > 2 else None
            body.append(_t(10, y + 16, label, 10.5, C_TXT_DIM, "start"))
            body.append(_t(w - 10, y + 16, "" if value is None else str(value),
                           10.5, C_TXT, "end"))
            if field and hits_out is not None:
                hits_out.append((0.0, y, float(w), PROP_ROW_H, field))
            y += PROP_ROW_H
    return _svg(w, h, "".join(body), panel_bg)


def text_bar(w, h, text, color=C_TXT_DIM, align="start", size=10.5, bg=C_CHROME):
    """窄条只读文本（命令标签 / 命令提示）：面板底色 + 一行对齐文字，无命中。"""
    anchor = {"start": "start", "center": "middle", "end": "end"}.get(align, "start")
    x = 10.0 if align == "start" else (w - 10 if align == "end" else w / 2.0)
    body = [_r(0, 0, w, h, bg), _t(x, h / 2.0 + 4, text, size, color, anchor)]
    return _svg(w, h, "".join(body), bg)
