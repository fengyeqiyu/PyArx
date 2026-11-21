# -*- coding: utf-8 -*-
"""控件层 flex 容器（XmlFlexPanel）测试。

和老套件 t_flex.py 的区别：那边量的是 XmlCore 外层算出来的 preview 矩形；
这里量的是"容器控件自己算、自己排"的真实子窗口矩形（win.get_rect），
也就是 HTML 的 flex 语义真的落在底层控件里，而不是宿主替它算。
"""
import os
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
# 允许用 PYARX_BIN 指向旁路编译目录（正在被占用的 X64\Release 不能覆盖时）
BIN = os.environ.get('PYARX_BIN') or os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')

import PyMfcXml as M  # noqa: E402

FAILS = []
CHECKS = [0]


def check(cond, what, got=None):
    CHECKS[0] += 1
    line = ('PASS ' if cond else 'FAIL ') + what
    if got is not None:
        line += '  got=%r' % (got,)
    print(line)
    if not cond:
        FAILS.append(what)
    return bool(cond)


def load(xml):
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    return win, names


def rect(win, names, key):
    return win.get_rect(names[key])


def real_rects(win, texts):
    """按窗口文本匹配真实子窗口矩形（GetWindowRect）。

    注意 names 回传的是 XmlCore 分配的 id，宿主建窗口时会另用一套 id，
    GetDlgItem 取不到，所以这里枚举子窗口按文本认人。
    容器层重排的验收必须走真实 HWND：win.get_rect 是 XmlCore 首帧算的缓存 preview，
    窗口尺寸变化后只有容器自己 reflowItems 才会挪动子窗口。
    """
    import ctypes
    import ctypes.wintypes as wt
    user32 = ctypes.windll.user32
    user32.EnumChildWindows.argtypes = [wt.HWND, ctypes.c_void_p, wt.LPARAM]
    user32.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
    user32.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
    found = {}
    CB = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)

    def cb(h, _):
        buf = ctypes.create_unicode_buffer(64)
        user32.GetWindowTextW(h, buf, 64)
        if buf.value in texts and buf.value not in found:
            r = wt.RECT()
            user32.GetWindowRect(h, ctypes.byref(r))
            found[buf.value] = (r.left, r.top, r.right - r.left, r.bottom - r.top)
        return True

    user32.EnumChildWindows(wt.HWND(int(win.hwnd)), CB(cb), 0)
    return found


def resize_window(win, w, h):
    import ctypes
    import ctypes.wintypes as wt
    user32 = ctypes.windll.user32
    user32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int,
                                   ctypes.c_int, ctypes.c_int, ctypes.c_uint]
    user32.SetWindowPos(wt.HWND(int(win.hwnd)), None, 0, 0, w, h, 0x0004 | 0x0010)
    M.pump(400)


def main():
    # ---- 1. row + gap：主轴递推 ----
    xml = """<XWindow Name="root" Width="520" Height="320">
  <FlexBox Name="p1" Width="400" Height="100" gap="10">
    <XLabel Name="a" Width="100" Height="30" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" Text="B"/>
    <XLabel Name="c" Width="100" Height="30" Text="C"/>
  </FlexBox>
</XWindow>"""
    win, names = load(xml)
    ra, rb, rc = rect(win, names, 'a'), rect(win, names, 'b'), rect(win, names, 'c')
    check(ra[0] == 0, '控件层 row 首项 x=0', ra)
    check(rb[0] == 110, '控件层 gap=10 第二项 x=110', rb)
    check(rc[0] == 220, '控件层 gap=10 第三项 x=220', rc)
    check(ra[1] == rb[1] == rc[1] == 0, '控件层 row 同排 y=0', (ra[1], rb[1], rc[1]))
    # CSS：align-items:stretch 只作用于"交叉轴尺寸为 auto"的子项，写了 Height 的不拉满
    check(ra[2] == 100 and ra[3] == 30, 'CSS align 默认 stretch 不覆盖显式 Height=30', ra)

    # ---- 2. justify-content: space-between ----
    xml2 = """<XWindow Name="root" Width="520" Height="320">
  <FlexBox Name="p2" Width="400" Height="100" gap="10" justify-content="space-between">
    <XLabel Name="a" Width="100" Height="30" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" Text="B"/>
    <XLabel Name="c" Width="100" Height="30" Text="C"/>
  </FlexBox>
</XWindow>"""
    win2, n2 = load(xml2)
    xa, xb, xc = rect(win2, n2, 'a')[0], rect(win2, n2, 'b')[0], rect(win2, n2, 'c')[0]
    check(xa == 0 and xb == 150 and xc == 300, '控件层 space-between 0/150/300', (xa, xb, xc))

    # ---- 3. flex-wrap：装不下就换行，行间距按 row-gap ----
    xml3 = """<XWindow Name="root" Width="520" Height="320">
  <FlexBox Name="p3" Width="250" Height="200" gap="10" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="100" Height="30" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" Text="B"/>
    <XLabel Name="c" Width="100" Height="30" Text="C"/>
  </FlexBox>
</XWindow>"""
    win3, n3 = load(xml3)
    a3, b3, c3 = rect(win3, n3, 'a'), rect(win3, n3, 'b'), rect(win3, n3, 'c')
    check(a3[1] == 0 and b3[1] == 0, 'wrap 前两项同排', (a3, b3))
    check(c3[1] == 40, 'wrap 第三项换到第二行 y=40', c3)
    check(c3[0] == 0, 'wrap 换行后从行首开始', c3)

    # ---- 4. align-self 覆盖容器的 align-items ----
    xml4 = """<XWindow Name="root" Width="520" Height="320">
  <FlexBox Name="p4" Width="400" Height="100" gap="10"><XLabel Name="a" Width="100" Height="30" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" align-self="center" Text="B"/>
  </FlexBox>
</XWindow>"""
    win4, n4 = load(xml4)
    check(rect(win4, n4, 'b')[1] == 35, 'align-self=center 居中 y=35', rect(win4, n4, 'b'))

    # ---- 5. column 方向 ----
    xml5 = """<XWindow Name="root" Width="520" Height="320">
  <FlexBox Name="p5" Width="300" Height="200" gap="10" flex-direction="column" align-items="start">
    <XLabel Name="a" Width="100" Height="30" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" Text="B"/>
    <XLabel Name="c" Width="100" Height="30" Text="C"/>
  </FlexBox>
</XWindow>"""
    win5, n5 = load(xml5)
    ya, yb, yc = rect(win5, n5, 'a')[1], rect(win5, n5, 'b')[1], rect(win5, n5, 'c')[1]
    check(ya == 0 and yb == 40 and yc == 80, '控件层 column 0/40/80', (ya, yb, yc))

    # ---- 6. flex-grow 按权重吃剩余主轴空间 ----
    xml6 = """<XWindow Name="root" Width="520" Height="320">
  <FlexBox Name="p6" Width="400" Height="100" gap="0">
    <XLabel Name="a" Width="100" Height="30" Grow="1" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" Grow="3" Text="B"/>
  </FlexBox>
</XWindow>"""
    win6, n6 = load(xml6)
    wa, wb = rect(win6, n6, 'a')[2], rect(win6, n6, 'b')[2]
    check(wa + wb == 400, 'grow 两项铺满 400', (wa, wb))
    # 剩余 200 按 1:3 分：100+50 / 100+150
    check(wa == 150 and wb == 250, 'grow 权重 1:3 -> 150/250', (wa, wb))

    for w in (win, win2, win3, win4, win5, win6):
        try:
            w.close()
        except Exception:
            pass

    # ---- 7. style="width:100%" 必须是百分比，不能读成 100px ----
    xml7 = """<XWindow Name="root" Width="520" Height="320">
  <XStackPanel Name="p7" Width="400" Height="100">
    <XLabel Name="a" Height="30" style="width:100%" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" Text="B"/>
  </XStackPanel>
</XWindow>"""
    win7, n7 = load(xml7)
    wa7, wb7 = rect(win7, n7, 'a')[2], rect(win7, n7, 'b')[2]
    check(wa7 == 400, 'style width:100% 铺满容器 400', wa7)
    check(wb7 == 100, 'style 未写尺寸的兄弟仍按 Width=100', wb7)
    try:
        win7.close()
    except Exception:
        pass

    # ---- 8. HTML 写法标签：<row> 横排、<column> 竖排 ----
    xml8 = """<XWindow Name="root" Width="520" Height="300">
  <row Name="r8" Width="400" Height="60">
    <XLabel Name="a8" Width="100" Text="A"/>
    <XLabel Name="b8" Width="100" Text="B"/>
  </row>
  <column Name="c8" Width="400" Height="120" Top="80">
    <XLabel Name="c$" Width="100" Text="C"/>
    <XLabel Name="d8" Width="100" Text="D"/>
  </column>
</XWindow>"""
    win8, n8 = load(xml8)
    ra8, rb8 = rect(win8, n8, 'a8'), rect(win8, n8, 'b8')
    check(ra8[0] == 0 and rb8[0] == 100, '<row> 横排 0/100', (ra8[0], rb8[0]))
    check(ra8[3] == 60 and rb8[3] == 60, '<row> stretch 拉满高 60', (ra8[3], rb8[3]))
    rc8, rd8 = rect(win8, n8, 'c$'), rect(win8, n8, 'd8')
    # 没写 Height 时按默认单行高（不做假定），只校验竖排顺序：d 紧贴 c 下沿
    check(rc8[1] == 80 and rd8[1] == rc8[1] + rc8[3],
          '<column> 竖排 y=80 / 紧贴上一项', (rc8[1], rd8[1], rc8[3]))
    check(rc8[2] == 100 and rd8[2] == 100, 'CSS <column> 显式 Width=100 不被 stretch 拉宽', (rc8[2], rd8[2]))
    try:
        win8.close()
    except Exception:
        pass

    # ---- 9. style 对任何元素生效（不只 HTML 标签）+ CSS 简写 ----
    xml9 = """<XWindow Name="root" Width="520" Height="300">
  <row Name="r9" Width="400" Height="60">
    <XLabel Name="a9" Width="100" Height="30" style="margin:8" Text="A"/>
    <XLabel Name="b9" Width="80" Height="30" style="width:80" Text="B"/>
  </row>
  <div Name="d9" Width="400" Height="60" style="margin:8">
    <XLabel Name="e9" Width="100" Height="30" Text="E"/>
  </div>
</XWindow>"""
    win9, n9 = load(xml9)
    r9 = rect(win9, n9, 'a9')
    check(r9[0] == 8 and r9[1] == 8, 'XLabel 上的 style="margin:8" 生效', (r9[0], r9[1]))
    rb = rect(win9, n9, 'b9')
    check(rb[2] == 80, 'XLabel 上的 style="width:80" 生效', rb[2])
    check(rb[0] == 116, 'margin 参与排布（前项右边距 8）', rb[0])
    re9 = rect(win9, n9, 'e9')
    check(re9[0] == 8 and re9[1] == 68, 'div 的 style="margin:8" 推子项（x=8，y=60+8）', (re9[0], re9[1]))
    try:
        win9.close()
    except Exception:
        pass

    # ---- 10. 容器自己重排：改窗口尺寸后由容器（而不是 XmlCore）重新换行 ----
    xml10 = """<XWindow Name="root" Width="520" Height="300" Background="#FF1E2226">
  <FlexBox Name="w10" Width="100%" Height="100%" gap="8" flex-wrap="wrap" align-items="start">
    <div Name="k1" style="width:200px;height:60px"><label>1</label></div>
    <div Name="k2" style="width:200px;height:60px"><label>2</label></div>
    <div Name="k3" style="width:200px;height:60px"><label>3</label></div>
    <div Name="k4" style="width:200px;height:60px"><label>4</label></div>
  </FlexBox>
</XWindow>"""
    win10, n10 = load(xml10)
    M.pump(300)
    b10 = real_rects(win10, {'1', '2', '3', '4'})
    check(len(b10) == 4, '四个卡片子窗口都真实存在', b10.keys())
    check(b10['1'][1] == b10['2'][1], '宽窗口 1/2 同一行', (b10['1'], b10['2']))
    check(b10['3'][1] == b10['1'][1] + 60 + 8, 'wrap 换行 + row-gap=8 -> 第二行 y 加 68',
          (b10['1'][1], b10['3'][1]))
    check(b10['1'][2] == 200, '显式 width:200px 不被 stretch 拉宽', b10['1'][2])
    resize_window(win10, 320, 300)
    a10 = real_rects(win10, {'1', '2', '3', '4'})
    check(a10['2'][1] - a10['1'][1] == 68, '容器缩窄后自己重排成一行一张（2 落到第二行）',
          (a10['1'], a10['2']))
    check(a10['3'][1] - a10['2'][1] == 68 and a10['4'][1] - a10['3'][1] == 68,
          '缩窄后 2/3/4 各占一行', (a10['2'], a10['3'], a10['4']))
    try:
        win10.close()
    except Exception:
        pass

    # ---- 11. CSS 尺寸上下限：min-width 不被 shrink 压穿，max-width 不被 grow 撑破 ----
    xml11 = """<XWindow Name="root" Width="420" Height="200">
  <FlexBox Name="p11" Width="404" Height="80" gap="10">
    <div Name="m1" style="width:200px;min-width:150px;height:60px"><label>1</label></div>
    <div Name="m2" style="width:200px;min-width:150px;height:60px"><label>2</label></div>
    <div Name="m3" style="width:200px;min-width:150px;height:60px"><label>3</label></div>
  </FlexBox>
</XWindow>"""
    win11, n11 = load(xml11)
    check(rect(win11, n11, 'm1')[2] == 150, 'min-width:150 阻止 shrink 把卡片压塌',
          rect(win11, n11, 'm1'))
    check(rect(win11, n11, 'm2')[2] == 150, '第二张同样守住 min-width', rect(win11, n11, 'm2'))
    try:
        win11.close()
    except Exception:
        pass

    xml11b = """<XWindow Name="root" Width="420" Height="200">
  <FlexBox Name="p11b" Width="404" Height="80" gap="10">
    <div Name="m4" style="max-width:120px;height:60px;flex:1"><label>4</label></div>
    <div Name="m5" style="height:60px;flex:1"><label>5</label></div>
  </FlexBox>
</XWindow>"""
    win11b, n11b = load(xml11b)
    m4, m5 = rect(win11b, n11b, 'm4'), rect(win11b, n11b, 'm5')
    check(m4[2] == 120, 'max-width:120 阻止 grow 撑破上限', m4)
    check(m5[2] == 274, '被 max-width 卡住的余量回池再分配（404-120-10=274）', m5)
    try:
        win11b.close()
    except Exception:
        pass

    # ---- 12. align-items: baseline —— 不同字号文字基线齐平，不是顶边对齐 ----
    xml12 = """<XWindow Name="root" Width="320" Height="110">
  <row Name="p12" Width="300" Height="70" gap="12" align="%s">
    <label Name="small12" Text="Small12" FontSize="12"/>
    <label Name="big24" Text="Big24" FontSize="24"/>
  </row>
</XWindow>"""
    pair = {}
    for mode in ('start', 'baseline'):
        w, nn = load(xml12 % mode)
        pair[mode] = (rect(w, nn, 'small12'), rect(w, nn, 'big24'))
        try:
            w.close()
        except Exception:
            pass
    s_start, b_start = pair['start']
    s_base, b_base = pair['baseline']
    check(s_start[1] == b_start[1], 'align=start 时两项顶边相同（基线不齐）', pair['start'])
    check(s_base[1] > b_base[1], 'align=baseline 时小字号下移、大字号上移', pair['baseline'])
    check(s_base[1] - b_base[1] == 16, '下移量 = 两者基线差（16px）', s_base[1] - b_base[1])

    # ---- 13. 第65轮补测：column + wrap 换列；align-content:stretch 行高均分 ----
    # 13a. column 方向换行：高不够时换到第二列；row-gap=8 管同列项距，column-gap=12 管列距
    xml13a = """<XWindow Name="root" Width="320" Height="300">
  <FlexBox Name="p13a" Width="200" Height="120" gap="8 12" flex-direction="column" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="80" Height="30" Text="A"/>
    <XLabel Name="b" Width="80" Height="30" Text="B"/>
    <XLabel Name="c" Width="80" Height="30" Text="C"/>
    <XLabel Name="d" Width="80" Height="30" Text="D"/>
    <XLabel Name="e" Width="80" Height="30" Text="E"/>
  </FlexBox>
</XWindow>"""
    win13a, n13a = load(xml13a)
    M.pump(200)
    r13 = real_rects(win13a, {"A", "B", "C", "D", "E"})
    check(r13["B"][1] - r13["A"][1] == 38 and r13["C"][1] - r13["B"][1] == 38,
          'column-wrap 同列 row-gap=8（项距 30+8=38）', (r13["A"], r13["B"], r13["C"]))
    check(r13["D"][0] - r13["A"][0] == 92,
          'column-wrap 第二列 column-gap=12（列距 80+12=92）', (r13["A"], r13["D"]))
    check(r13["D"][1] == r13["A"][1] and r13["E"][1] - r13["D"][1] == 38,
          '第二列从列首起排、项距同为 38', (r13["A"], r13["D"], r13["E"]))
    try:
        win13a.close()
    except Exception:
        pass

    # 13b. align-content:stretch：两行均分剩余交叉空间，auto 高度子项拉满行高
    xml13b = """<XWindow Name="root" Width="420" Height="200">
  <FlexBox Name="p13b" Width="400" Height="120" flex-wrap="wrap" align-content="stretch">
    <XLabel Name="s1" Width="150" Text="1"/>
    <XLabel Name="s2" Width="150" Text="2"/>
    <XLabel Name="s3" Width="150" Text="3"/>
  </FlexBox>
</XWindow>"""
    win13b, n13b = load(xml13b)
    M.pump(200)
    rs = real_rects(win13b, {"1", "2", "3"})
    check(rs["1"][3] == 60 and rs["2"][3] == 60 and rs["3"][3] == 60,
          'stretch 两行均分 120，子项拉满行高 60', rs)
    check(rs["3"][1] - rs["1"][1] == 60, 'stretch 第二行落在 y=60', (rs["1"], rs["3"]))
    try:
        win13b.close()
    except Exception:
        pass

    # ---- 14. 第66轮：wrap 容器 auto 高度按行累加撑高；显式 Height 不被撑大 ----
    # 14a. 不写 Height：400 宽放 3 个 150x60，折两行 -> 容器高 = 60+60 = 120
    xml14a = """<XWindow Name="root" Width="420" Height="300">
  <FlexBox Name="p14a" Width="400" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="150" Height="60" Text="A"/>
    <XLabel Name="b" Width="150" Height="60" Text="B"/>
    <XLabel Name="c" Width="150" Height="60" Text="C"/>
  </FlexBox>
</XWindow>"""
    win14a, n14a = load(xml14a)
    M.pump(200)
    ra14 = rect(win14a, n14a, 'p14a')
    rc14 = rect(win14a, n14a, 'c')
    check(ra14[3] == 120, 'auto 高两行累加 = 120', ra14)
    check(rc14[1] == 60 and rc14[3] == 60, '第二行子项 y=60、高 60', rc14)
    try:
        win14a.close()
    except Exception:
        pass

    # 14b. 行间距计入 auto 高：gap=10 -> 60+10+60 = 130
    xml14b = """<XWindow Name="root" Width="420" Height="300">
  <FlexBox Name="p14b" Width="400" gap="10" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="150" Height="60" Text="A"/>
    <XLabel Name="b" Width="150" Height="60" Text="B"/>
    <XLabel Name="c" Width="150" Height="60" Text="C"/>
  </FlexBox>
</XWindow>"""
    win14b, n14b = load(xml14b)
    M.pump(200)
    check(rect(win14b, n14b, 'p14b')[3] == 130, 'auto 高含 row-gap=10 -> 130',
          rect(win14b, n14b, 'p14b'))
    try:
        win14b.close()
    except Exception:
        pass

    # 14c. 显式 Height=24：容器不被第二行撑大，子项仍 24 高（防回归）
    xml14c = """<XWindow Name="root" Width="420" Height="300">
  <FlexBox Name="p14c" Width="400" Height="24" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="150" Height="24" Text="A"/>
    <XLabel Name="b" Width="150" Height="24" Text="B"/>
    <XLabel Name="c" Width="150" Height="24" Text="C"/>
  </FlexBox>
</XWindow>"""
    win14c, n14c = load(xml14c)
    M.pump(200)
    rc14c = rect(win14c, n14c, 'p14c')
    check(rc14c[3] == 24, '显式 Height=24 容器不被撑大（非 48）', rc14c)
    check(rect(win14c, n14c, 'a')[3] == 24, '显式高度子项仍 24 高', rect(win14c, n14c, 'a'))
    try:
        win14c.close()
    except Exception:
        pass

    # 14d. wrap-reverse 的 auto 高同口径（行上下翻转不改变累计高度）
    xml14d = """<XWindow Name="root" Width="420" Height="300">
  <FlexBox Name="p14d" Width="400" flex-wrap="wrap-reverse" align-items="start">
    <XLabel Name="a" Width="150" Height="60" Text="A"/>
    <XLabel Name="b" Width="150" Height="60" Text="B"/>
    <XLabel Name="c" Width="150" Height="60" Text="C"/>
  </FlexBox>
</XWindow>"""
    win14d, n14d = load(xml14d)
    M.pump(200)
    check(rect(win14d, n14d, 'p14d')[3] == 120, 'wrap-reverse auto 高同为 120',
          rect(win14d, n14d, 'p14d'))
    try:
        win14d.close()
    except Exception:
        pass

    # ---- 15. 第67轮：column + wrap 的 auto 宽度（写死可用高后按拆列结果撑开）----
    # 口径推算：容器高 120、子项高 60、row-gap=0 -> 一列正好放 120/60=2 项；
    # 3 项折成 2 列。column 方向 gap 分工：row-gap(m_gapY) 管列内纵向项距，
    # column-gap(m_gapX) 管列间横向距。
    # 15a. 不写 gap：宽 = 2×150 + 0 = 300；第三项进第二列 x=150
    xml15a = """<XWindow Name="root" Width="500" Height="300">
  <FlexBox Name="p15a" Height="120" flex-direction="column" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="150" Height="60" Text="A"/>
    <XLabel Name="b" Width="150" Height="60" Text="B"/>
    <XLabel Name="c" Width="150" Height="60" Text="C"/>
  </FlexBox>
</XWindow>"""
    win15a, n15a = load(xml15a)
    M.pump(200)
    rp15a = rect(win15a, n15a, 'p15a')
    rc15a = rect(win15a, n15a, 'c')
    check(rp15a[2] == 300, 'column-wrap auto 宽 = 2 列×150 = 300', rp15a)
    check(rc15a[0] == 150, '第三项进第二列 x=150', rc15a)
    try:
        win15a.close()
    except Exception:
        pass

    # 15b. gap="0 12"：row-gap=0（列内仍 2 项/列），column-gap=12 -> 宽 = 150+12+150 = 312；
    # 第二列起点 x = 150+12 = 162
    xml15b = """<XWindow Name="root" Width="500" Height="300">
  <FlexBox Name="p15b" Height="120" gap="0 12" flex-direction="column" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="150" Height="60" Text="A"/>
    <XLabel Name="b" Width="150" Height="60" Text="B"/>
    <XLabel Name="c" Width="150" Height="60" Text="C"/>
  </FlexBox>
</XWindow>"""
    win15b, n15b = load(xml15b)
    M.pump(200)
    rp15b = rect(win15b, n15b, 'p15b')
    rc15b = rect(win15b, n15b, 'c')
    check(rp15b[2] == 312, 'column-wrap auto 宽含 column-gap=12 -> 312', rp15b)
    check(rc15b[0] == 162, '第二列起点 x=150+12=162', rc15b)
    try:
        win15b.close()
    except Exception:
        pass

    # 15c. 显式 Width=150：作者尺寸优先，容器宽保持 150 不被撑开；第三项溢出到第二列 x=150
    xml15c = """<XWindow Name="root" Width="500" Height="300">
  <FlexBox Name="p15c" Width="150" Height="120" flex-direction="column" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="150" Height="60" Text="A"/>
    <XLabel Name="b" Width="150" Height="60" Text="B"/>
    <XLabel Name="c" Width="150" Height="60" Text="C"/>
  </FlexBox>
</XWindow>"""
    win15c, n15c = load(xml15c)
    M.pump(200)
    rp15c = rect(win15c, n15c, 'p15c')
    check(rp15c[2] == 150, '显式 Width=150 容器不被撑开（非 300）', rp15c)
    try:
        win15c.close()
    except Exception:
        pass

    # ---- 16. 第68轮：flex 容器真父子层级（SetParent 到容器 + 平移跟随 + 裁剪几何）----
    import ctypes as _ct
    import ctypes.wintypes as _wt
    _u = _ct.windll.user32
    _u.GetParent.argtypes = [_wt.HWND]; _u.GetParent.restype = _wt.HWND
    _u.GetWindowRect.argtypes = [_wt.HWND, _ct.POINTER(_wt.RECT)]
    _u.GetWindowTextW.argtypes = [_wt.HWND, _wt.LPWSTR, _ct.c_int]
    _u.GetWindowLongPtrW.argtypes = [_wt.HWND, _ct.c_int]; _u.GetWindowLongPtrW.restype = _ct.c_void_p
    _WS_CLIPCHILDREN = 0x02000000

    def _enum_child_hwnds(win):
        found = {}
        CB = _ct.WINFUNCTYPE(_wt.BOOL, _wt.HWND, _wt.LPARAM)
        def cb(h, _):
            buf = _ct.create_unicode_buffer(64)
            _u.GetWindowTextW(h, buf, 64)
            found[buf.value or ('static%d' % int(h))] = int(h)
            return True
        _u.EnumChildWindows(_wt.HWND(int(win.hwnd)), CB(cb), 0)
        return found

    def _screen(hwnd):
        r = _wt.RECT(); _u.GetWindowRect(_wt.HWND(hwnd), _ct.byref(r))
        return (r.left, r.top, r.right - r.left, r.bottom - r.top)

    # 16a. 子项 GetParent == 容器 hwnd（不是宿主扁平兄弟），容器父仍是根窗口
    xml16a = """<XWindow Name="root" Width="420" Height="300">
  <FlexBox Name="p16" Width="300" Height="120" gap="10" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="100" Height="30" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" Text="B"/>
    <XLabel Name="c" Width="100" Height="30" Text="C"/>
  </FlexBox>
</XWindow>"""
    win16, n16 = load(xml16a)
    M.pump(200)
    hw16 = _enum_child_hwnds(win16)
    root16 = int(win16.hwnd)
    box16 = int(_u.GetParent(_wt.HWND(hw16['A'])) or 0)
    par_b = int(_u.GetParent(_wt.HWND(hw16['B'])) or 0)
    par_c = int(_u.GetParent(_wt.HWND(hw16['C'])) or 0)
    check(box16 != root16 and box16 == par_b == par_c,
          '16a 子项 GetParent == 容器 hwnd（真父子，非扁平兄弟）', hex(box16))
    # 父链：root -> 宿主内容窗 -> 容器（容器直接父是宿主中间层，根是祖先）
    _anc, _in = box16, False
    while _anc:
        if _anc == root16:
            _in = True
        _anc = int(_u.GetParent(_wt.HWND(_anc)) or 0)
    check(_in, '16a 容器在根窗口祖先链上', hex(box16))

    # 16b. 容器经宿主通道平移 +40px，子项屏幕坐标同步 +40、纵向不动（靠 OS 父子层级跟随，不经 reflow 推坐标）
    before16 = {k: _screen(hw16[k]) for k in ('A', 'B', 'C')}
    rb16 = rect(win16, n16, 'p16')
    win16.move(n16['p16'], rb16[0] + 40, rb16[1], rb16[2], rb16[3])
    M.pump(200)
    ok_follow = True
    got_follow = {}
    for k in ('A', 'B', 'C'):
        after = _screen(hw16[k])
        got_follow[k] = (after[0] - before16[k][0], after[1] - before16[k][1])
        if got_follow[k] != (40, 0):
            ok_follow = False
    check(ok_follow, '16b 容器平移 +40 子项同步跟随 dx=40 dy=0', got_follow)
    try:
        win16.close()
    except Exception:
        pass

    # 16c. 裁剪几何口径：固定 Height=30 放不下第二行，C 溢出到容器底外；
    #      子窗口由系统裁剪到父客户区，容器带 WS_CLIPCHILDREN；几何上 C 底边在容器外，绘制不溢出。
    xml16c = """<XWindow Name="root" Width="420" Height="300">
  <FlexBox Name="p16c" Width="200" Height="30" flex-wrap="wrap" align-items="start">
    <XLabel Name="a" Width="100" Height="30" Text="A"/>
    <XLabel Name="b" Width="100" Height="30" Text="B"/>
    <XLabel Name="c" Width="100" Height="30" Text="C"/>
  </FlexBox>
</XWindow>"""
    win16c, n16c = load(xml16c)
    M.pump(200)
    hw16c = _enum_child_hwnds(win16c)
    box16c = int(_u.GetParent(_wt.HWND(hw16c['C'])) or 0)
    br16c = _screen(box16c)
    cr16c = _screen(hw16c['C'])
    style16c = int(_u.GetWindowLongPtrW(_wt.HWND(box16c), -16) or 0)
    check(cr16c[1] + cr16c[3] > br16c[1] + br16c[3],
          '16c 第二行子项几何上溢出容器底边（待系统裁剪）', (cr16c, br16c))
    check((style16c & _WS_CLIPCHILDREN) != 0, '16c 容器带 WS_CLIPCHILDREN', hex(style16c))
    try:
        win16c.close()
    except Exception:
        pass

    print('\nt_flexpanel: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
    for f in FAILS:
        print('  - ' + f)
    return 1 if FAILS else 0


if __name__ == '__main__':
    sys.exit(main())
