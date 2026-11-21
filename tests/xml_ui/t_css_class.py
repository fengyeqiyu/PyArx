# -*- coding: utf-8 -*-
"""CSS 规则表：<style>.class{...}</style> 与 class="a b"，等价于 HTML 的复用写法。

HTML 里写一遍 .card{...} 就能给 N 个元素同一套外观；XML 侧此前只能在每个元素上
抄一遍 style="..."。这里校验：class 命中、多 class 叠加、#id、tag.class、
逗号并列、行内 style 覆盖 class。
"""
import os
import sys

from _kit import M, check, finish

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LOG_DIR = os.path.join(ROOT, 'log')
os.makedirs(LOG_DIR, exist_ok=True)


def load(xml):
    names = {}
    win = M.guiload_text(xml, context=globals(), names=names)
    win.show()
    M.pump(120)
    return win, names


def main():
    css = r'''
     /* 注释不应把后面的规则吃掉 */
     .card { width:160px; height:60px; background:#FF0000; border-radius:8px }
     .card.wide { width:300px }
     #only { height:100px }
     div.plain { padding:6px; width:100px; height:40px }
     div.flat, span.flat { height:20px }
    '''
    xml = ('<?xml version="1.0"?><XWindow Width="400" Height="300">'
           '<style>' + css + '</style>'
           '<div id="a" class="card"></div>'
           '<div id="b" class="card wide"></div>'
           '<div id="only" class="card wide"></div>'
           '<div id="c" class="card" style="width:200px"></div>'
           '<div id="p" class="plain"></div>'
           '</XWindow>')
    win, names = load(xml)

    def rect(k):
        return win.get_rect(names[k])

    check(rect('a') == (0, 0, 160, 60), '.card 命中：一条规则给多个元素', rect('a'))
    check(rect('b') == (0, 60, 300, 60), '多 class 叠加（.card.wide 更具体）', rect('b'))
    check(rect('only') == (0, 120, 300, 100), '#id 规则命中且不覆盖 class 的 width', rect('only'))
    check(rect('c') == (0, 220, 200, 60), '行内 style 覆盖 class 声明', rect('c'))
    check(rect('p') == (0, 280, 100, 40), 'tag.class 命中（div.plain 的 padding 生效）', rect('p'))

    # <style> 节点本身不产生界面：窗口里只应有 5 个 div
    check(len(names) == 5, '<style> 不产生控件', sorted(names.keys()))
    win.save_png(r'G:\PyArx\log\after\t_css_class.png')
    win.close()

    # ---- 伪类：`:hover` / `:active` / `:focus` 在 <style> 里写，落到等价状态属性
    xml2 = ('<?xml version="1.0"?><XWindow Width="360" Height="200">'
            '<style>'
            ' .btn { padding:6px; background:#EFEFEF; color:#222 }'
            ' .btn:hover { background:#1E90FF; color:#FFFFFF }'
            ' .btn:active { background:#0A58CA }'
            ' .btn:focus { color:#000000 }'
            '</style>'
            '<button id="ok" class="btn">确定</button>'
            '<button id="plain">普通</button>'
            '</XWindow>')
    win2, n2 = load(xml2)
    ok = lambda k: str(win2.get_xml_value(n2['ok'], k))
    pl = lambda k: str(win2.get_xml_value(n2['plain'], k))
    check(ok('HoverBackground') == '#1E90FF', '.btn:hover 背景落到 HoverBackground', ok('HoverBackground'))
    check(ok('HoverColor') == '#FFFFFF' and ok('FocusColor') == '#000000',
          ':hover 前景 / :focus 前景各自落地', (ok('HoverColor'), ok('FocusColor')))
    check(ok('ActiveBackground') == '#0A58CA', ':active 背景落到 ActiveBackground', ok('ActiveBackground'))
    check(pl('HoverBackground') == '', '没挂 class 的控件不受规则影响', pl('HoverBackground'))
    win2.close()

    # ---- 权重：id > class > tag，与源码先后无关
    xml3 = ('<?xml version="1.0"?><XWindow Width="500" Height="120">'
            '<style>'
            ' #x { width:300px; height:40px }'
            ' div { width:100px; height:40px; background:#DDD }'
            ' .card { width:160px }'
            '</style>'
            '<div id="s1" class="card"></div>'
            '<div id="x" class="card"></div>'
            '</XWindow>')
    win3, n3 = load(xml3)
    check(win3.get_rect(n3['s1'])[2] == 160, 'class 赢过后写的 tag 规则（160 而非 100）',
          win3.get_rect(n3['s1']))
    check(win3.get_rect(n3['x'])[2] == 300, 'id 规则赢过 class（300 而非 160）',
          win3.get_rect(n3['x']))
    win3.close()

    # ---- 层级选择器：.card .title（后代）与 .card > .body（子代）
    xml4 = ('<?xml version="1.0"?><XWindow Width="520" Height="200">'
            '<style>'
            ' .card { background:#EEE }'
            ' .card .title { width:120px; height:20px }'
            ' .card > .body { width:180px; height:20px }'
            ' div .deep { width:60px; height:20px }'
            '</style>'
            '<div id="card" class="card" style="width:300px;height:140px">'
            '<div id="t" class="title"></div>'
            '<div id="b1" class="body"></div>'
            '<div id="mid"><div id="b2" class="body"></div></div>'
            '<div id="d" class="deep"></div>'
            '</div>'
            '</XWindow>')
    win4, n4 = load(xml4)
    check(win4.get_rect(n4['t'])[2] == 120, '.card .title 后代规则命中（120）',
          win4.get_rect(n4['t']))
    check(win4.get_rect(n4['b1'])[2] == 180, '.card > .body 子代规则命中（180）',
          win4.get_rect(n4['b1']))
    check(win4.get_rect(n4['b2'])[2] != 180, '.card > .body 不命中隔了一层的 .body',
          win4.get_rect(n4['b2']))
    check(win4.get_rect(n4['d'])[2] == 60, 'div .deep 跨层后代规则命中（60）',
          win4.get_rect(n4['d']))
    win4.close()

    # ---- 兄弟组合器：`+` 紧邻、`~` 后继（跨过中间元素）
    xml5 = ('<?xml version="1.0"?><XWindow Width="460" Height="140">'
            '<style>'
            ' #lab ~ .ctl { width:150px; height:20px }'
            ' #lab + .ctl { width:120px }'
            '</style>'
            '<div id="row" style="width:400px;height:100px">'
            '<div id="lab" style="width:80px;height:20px"></div>'
            '<div id="in1" class="ctl"></div>'
            '<div id="mid" style="width:80px;height:20px"></div>'
            '<div id="in2" class="ctl"></div>'
            '</div>'
            '</XWindow>')
    win5, n5 = load(xml5)
    check(win5.get_rect(n5['in1'])[2] == 120, '#lab + .ctl 紧邻兄弟命中（120）',
          win5.get_rect(n5['in1']))
    check(win5.get_rect(n5['in2'])[2] == 150, '#lab ~ .ctl 跨过 #mid 的后继兄弟命中（150）',
          win5.get_rect(n5['in2']))
    win5.close()

    # ---- 属性选择器：input[type="text"] 与 [data-role]
    xml6 = ('<?xml version="1.0"?><XWindow Width="520" Height="160">'
            '<style>'
            ' input[type="text"] { width:180px; height:24px }'
            ' [data-role] { height:20px; width:90px }'
            '</style>'
            '<div id="t1"><input id="i1" type="text"/></div>'
            '<div id="t2"><input id="i2" type="checkbox"/></div>'
            '<div id="r1" data-role="tag"></div>'
            '</XWindow>')
    win6, n6 = load(xml6)
    check(win6.get_rect(n6['i1'])[2] == 180, 'input[type="text"] 精确命中（180）',
          win6.get_rect(n6['i1']))
    check(win6.get_rect(n6['i2'])[2] != 180, 'input[type="checkbox"] 不被 text 规则命中',
          win6.get_rect(n6['i2']))
    check(win6.get_rect(n6['r1'])[2] == 90, '[data-role] 仅查属性存在也命中（90）',
          win6.get_rect(n6['r1']))
    win6.close()

    # ---- 结构伪类：.row:nth-child(odd/even) 斑马纹 + :first-child
    xml7 = ('<?xml version="1.0"?><XWindow Width="420" Height="200">'
            '<style>'
            ' .row { width:100px; height:20px }'
            ' .row:nth-child(odd) { width:120px }'
            ' .row:nth-child(even) { width:180px }'
            ' .row:first-child { height:30px }'
            '</style>'
            '<div id="list" style="width:400px;height:180px">'
            '<div id="r1" class="row"></div>'
            '<div id="r2" class="row"></div>'
            '<div id="r3" class="row"></div>'
            '<div id="r4" class="row"></div>'
            '</div>'
            '</XWindow>')
    win7, n7 = load(xml7)
    check(win7.get_rect(n7['r1'])[2] == 120, ':nth-child(odd) 第 1 行（120）',
          win7.get_rect(n7['r1']))
    check(win7.get_rect(n7['r2'])[2] == 180, ':nth-child(even) 第 2 行（180）',
          win7.get_rect(n7['r2']))
    check(win7.get_rect(n7['r3'])[2] == 120, ':nth-child(odd) 第 3 行（120）',
          win7.get_rect(n7['r3']))
    check(win7.get_rect(n7['r1'])[3] == 30, ':first-child 高度 30',
          win7.get_rect(n7['r1']))
    win7.close()

    # ---- an+b：`.r:nth-child(2n+1)` / `:nth-child(-n+2)`
    xml8 = ('<?xml version="1.0"?><XWindow Width="460" Height="240">'
            '<style>'
            ' .r { width:100px; height:20px }'
            ' .r:nth-child(2n+1) { width:140px }'
            ' .r:nth-child(-n+2) { height:26px }'
            '</style>'
            '<div id="box" style="width:440px;height:220px">'
            '<div id="a1" class="r"></div>'
            '<div id="a2" class="r"></div>'
            '<div id="a3" class="r"></div>'
            '<div id="a4" class="r"></div>'
            '</div>'
            '</XWindow>')
    win8, n8 = load(xml8)
    check(win8.get_rect(n8['a1'])[2] == 140, '2n+1 命中第 1 行（140）', win8.get_rect(n8['a1']))
    check(win8.get_rect(n8['a2'])[2] == 100, '2n+1 不命中第 2 行（100）', win8.get_rect(n8['a2']))
    check(win8.get_rect(n8['a3'])[2] == 140, '2n+1 命中第 3 行（140）', win8.get_rect(n8['a3']))
    check(win8.get_rect(n8['a2'])[3] == 26, '-n+2 命中第 2 行高度 26', win8.get_rect(n8['a2']))
    check(win8.get_rect(n8['a3'])[3] == 20, '-n+2 不命中第 3 行（20）', win8.get_rect(n8['a3']))
    win8.close()

    # ---- :not() 排除：`.btn:not(.primary)` 只给非主按钮加样式
    xml9 = ('<?xml version="1.0"?><XWindow Width="420" Height="120">'
            '<style>'
            ' .btn { width:80px; height:24px }'
            ' .btn:not(.primary) { width:120px }'
            '</style>'
            '<div id="v"><div id="b1" class="btn"></div>'
            '<div id="b2" class="btn primary"></div></div>'
            '</XWindow>')
    win9, n9 = load(xml9)
    check(win9.get_rect(n9['b1'])[2] == 120, ':not(.primary) 命中普通按钮（120）',
          win9.get_rect(n9['b1']))
    check(win9.get_rect(n9['b2'])[2] == 80, ':not(.primary) 排除主按钮（80）',
          win9.get_rect(n9['b2']))
    win9.close()

    # ---- !important：低权重声明也要压过高权重普通声明（HTML 里绕开优先级的标准手段）
    xml10 = ('<?xml version="1.0"?><XWindow Width="420" Height="120">'
             '<style>'
             ' #only { width:50px; height:24px }'
             ' div { width:200px !important }'
             '</style>'
             '<div id="v"><div id="c1" id2="x" class="card"></div>'
             '<div id="only"></div></div>'
             '</XWindow>')
    winA, nA = load(xml10)
    check(winA.get_rect(nA['c1'])[2] == 200, '!important 压过常规（div 200 胜 .card 160）',
          winA.get_rect(nA['c1']))
    check(winA.get_rect(nA['only'])[2] == 200, '!important 压过 #id 常规声明（200 非 50）',
          winA.get_rect(nA['only']))
    winA.close()

    # ---- :disabled / :checked：HTML 里 `input:disabled{}` 直接给禁用态换皮，
    #      XML 侧此前只能逐个元素写 disabled 专属样式（或干脆不写）
    xml11 = ('<?xml version="1.0"?><XWindow Width="420" Height="140">'
             '<style>'
             ' input { width:100px; height:24px }'
             ' input:disabled { width:200px; background-color:#CCCCCC; color:#777777 }'
             ' input:checked { width:60px }'
             '</style>'
             '<div id="v">'
             '<input id="i1"/>'
             '<input id="i2" disabled="1"/>'
             '<input id="i3" disabled="0"/>'
             '<input id="i4" type="checkbox" checked="1"/>'
             '</div></XWindow>')
    winB, nB = load(xml11)
    check(winB.get_rect(nB['i1'])[2] == 100, '常态 input 不吃 :disabled 规则（100）',
          winB.get_rect(nB['i1']))
    check(winB.get_rect(nB['i2'])[2] == 200, 'disabled="1" 命中 :disabled（200）',
          winB.get_rect(nB['i2']))
    check(winB.get_rect(nB['i3'])[2] == 100, 'disabled="0" 不命中 :disabled（100）',
          winB.get_rect(nB['i3']))
    check(winB.get_rect(nB['i4'])[2] == 60, 'checked="1" 命中 :checked（60）',
          winB.get_rect(nB['i4']))
    # 外观类声明走状态属性（几何仍静态合）：禁用态换皮在运行时 EnableWindow 后也生效
    check(winB.get_xml_value(nB['i2'], 'DisabledBackground') == '#CCCCCC',
          ':disabled 的底色落到 DisabledBackground',
          winB.get_xml_value(nB['i2'], 'DisabledBackground'))
    check(winB.get_xml_value(nB['i2'], 'DisabledColor') == '#777777',
          ':disabled 的文字色落到 DisabledColor',
          winB.get_xml_value(nB['i2'], 'DisabledColor'))
    check(winB.get_xml_value(nB['i1'], 'DisabledBackground') == '',
          '未禁用的元素不吃 :disabled 外观', winB.get_xml_value(nB['i1'], 'DisabledBackground'))
    # HTML 的 `disabled` 必须真的禁用（WS_DISABLED），不只是选择器命中
    check(winB.get_enabled(nB['i2']) is False, 'disabled="1" 真的把控件禁用',
          winB.get_enabled(nB['i2']))
    check(winB.get_enabled(nB['i1']) is True, '未写 disabled 的控件仍可用',
          winB.get_enabled(nB['i1']))
    # 归一表：XML 原生标签上 `disabled` / `readonly` 与 XML 内部名等价（同一处消费者）
    from _kit import children as _children
    import ctypes as _ct
    xmlAlias = ('<?xml version="1.0"?><XWindow Width="420" Height="220">'
                '<XButton Name="ax" Text="a" Width="120" Height="40" disabled="1"/>'
                '<XButton Name="ay" Text="b" Width="120" Height="40" Enabled="True"/>'
                '<XTextField Name="e1" Width="160" Height="26" readonly="1"/>'
                '<XTextField Name="e2" Width="160" Height="26"/>'
                '<XTextField Name="e3" Width="160" Height="26" IsReadOnly="1"/>'
                '<XTextField Name="e4" Width="160" Height="26" IsReadOnly="0"/>'
                '</XWindow>')
    winK, nK = load(xmlAlias)
    kids = _children(__import__('_kit').hwnd_of(winK))
    gwl = _ct.windll.user32.GetWindowLongPtrW
    check(winK.get_enabled(nK['ax']) is False, 'XML 标签上 disabled="1" 也真禁用',
          winK.get_enabled(nK['ax']))
    check(winK.get_enabled(nK['ay']) is True, 'XML 标签上 Enabled="True" 仍可用',
          winK.get_enabled(nK['ay']))
    st1 = gwl(kids[nK['e1']][0], -16)
    st2 = gwl(kids[nK['e2']][0], -16)
    st3 = gwl(kids[nK['e3']][0], -16)
    st4 = gwl(kids[nK['e4']][0], -16)
    check(bool(st1 & 0x0800) and not (st2 & 0x0800),
          'XML 标签上 readonly="1" 真只读（ES_READONLY 已置）', (hex(st1), hex(st2)))
    check(bool(st3 & 0x0800) and not (st4 & 0x0800),
          '内部名 IsReadOnly 与别名等价（1 只读 / 0 可编辑）', (hex(st3), hex(st4)))
    winK.close()
    winB.close()

    # ---- @media：窄屏/宽屏两套尺寸，HTML 里靠媒体查询，XML 侧只能重生整棵树
    xml12 = ('<?xml version="1.0"?><XWindow Width="500" Height="140">'
             '<style>'
             ' .card { width:300px; height:24px }'
             ' @media (max-width:600px) { .card { width:200px } }'
             ' @media (min-width:700px) { .card { width:100px } }'
             '</style>'
             '<div id="v"><div id="c9" class="card"></div></div></XWindow>')
    winC, nC = load(xml12)
    check(winC.get_rect(nC['c9'])[2] == 200, '窄窗口命中 @media(max-width:600px)（200）',
          winC.get_rect(nC['c9']))
    winC.close()

    xml13 = ('<?xml version="1.0"?><XWindow Width="800" Height="140">'
             '<style>'
             ' .card { width:300px; height:24px }'
             ' @media (max-width:600px) { .card { width:200px } }'
             ' @media (min-width:700px) { .card { width:100px } }'
             '</style>'
             '<div id="v"><div id="c9" class="card"></div></div></XWindow>')
    winD, nD = load(xml13)
    check(winD.get_rect(nD['c9'])[2] == 100, '宽窗口命中 @media(min-width:700px)（100）',
          winD.get_rect(nD['c9']))
    winD.close()

    # ---- ::before / ::after：HTML 的 `content:"★ "` 前缀/后缀，XML 只能写进正文
    xml14 = ('<?xml version="1.0"?><XWindow Width="420" Height="140">'
             '<style>'
             ' .tag::before { content:"★ " }'
             ' .tag::after { content:" (必读)" }'
             ' .tag { font-size:12px }'
             '</style>'
             '<div id="v"><label id="t1" class="tag">重点</label>'
             '<label id="t2">普通</label></div></XWindow>')
    winE, nE = load(xml14)
    check(winE.get_text(nE['t1']) == '★ 重点 (必读)',
          '::before/::after 拼出前缀与后缀', winE.get_text(nE['t1']))
    check(winE.get_text(nE['t2']) == '普通',
          '未命中伪元素的控件文字不变', winE.get_text(nE['t2']))
    winE.close()

    # ---- content:attr(x)：HTML 里把元素自己的属性值印进伪元素，属性改一次文字跟着变
    xml15 = ('<?xml version="1.0"?><XWindow Width="420" Height="150">'
             '<div id="v">'
             '<label id="k1" class="chip" data-tag="A">甲</label>'
             '<label id="k2" class="chip" data-tag="B">乙</label>'
             '<label id="k3">丙</label></div>'
             '<style> .chip::before { content:"[" attr(data-tag) "] " } </style></XWindow>')
    winF, nF = load(xml15)
    check(winF.get_text(nF['k1']) == '[A] 甲', 'content:attr() 印出属性值（A）',
          winF.get_text(nF['k1']))
    check(winF.get_text(nF['k2']) == '[B] 乙', 'content:attr() 随属性值变化（B）',
          winF.get_text(nF['k2']))
    check(winF.get_text(nF['k3']) == '丙', '无属性/未命中规则的文字不变', winF.get_text(nF['k3']))
    winF.close()

    # ---- 状态化边框色：`button:hover{border-color:#fff}` 只换线色、不改几何
    xml16 = ('<?xml version="1.0"?><XWindow Width="320" Height="160">'
             '<XButton Name="hb" Text="btn" Width="120" Height="40" X="20" Y="20"'
             ' style="border:2px solid #333333"/>'
             '<style>'
             ' #hb:hover { border-color:#00ff00 }'
             ' #hb:focus { border-color:#0088ff }'
             ' #hb:active { border-color:#ff0000 }'
             '</style></XWindow>')
    winG, nG = load(xml16)
    check(winG.get_xml_value(nG['hb'], 'HoverBorderColor') == '#00ff00',
          ':hover 的 border-color 落到 HoverBorderColor',
          winG.get_xml_value(nG['hb'], 'HoverBorderColor'))
    check(winG.get_xml_value(nG['hb'], 'FocusBorderColor') == '#0088ff',
          ':focus 的 border-color 落到 FocusBorderColor',
          winG.get_xml_value(nG['hb'], 'FocusBorderColor'))
    check(winG.get_xml_value(nG['hb'], 'ActiveBorderColor') == '#ff0000',
          ':active 的 border-color 落到 ActiveBorderColor',
          winG.get_xml_value(nG['hb'], 'ActiveBorderColor'))
    check(winG.get_xml_value(nG['hb'], 'BorderColor') == '#333333',
          '常态边框色不被状态规则改掉（只换状态）',
          winG.get_xml_value(nG['hb'], 'BorderColor'))
    winG.close()

    # ---- 状态化不透明度：`:hover{opacity:.5}` 只改绘制不改几何
    xml17 = ('<?xml version="1.0"?><XWindow Width="320" Height="160">'
             '<XButton Name="ob" Text="btn" Width="120" Height="40" X="20" Y="20"/>'
             '<style> #ob:hover { opacity:0.5 } #ob:focus { opacity:0.8 } </style></XWindow>')
    winH, nH = load(xml17)
    check(winH.get_xml_value(nH['ob'], 'HoverOpacity') == '128',
          ':hover 的 opacity 落到 HoverOpacity（0.5→128）',
          winH.get_xml_value(nH['ob'], 'HoverOpacity'))
    check(winH.get_xml_value(nH['ob'], 'FocusOpacity') == '204',
          ':focus 的 opacity 落到 FocusOpacity（0.8→204）',
          winH.get_xml_value(nH['ob'], 'FocusOpacity'))
    check(winH.get_xml_value(nH['ob'], 'Opacity') == '255',
          '常态 opacity 不被状态规则改掉', winH.get_xml_value(nH['ob'], 'Opacity'))
    winH.close()

    # ---- 状态化 transform：`:hover{transform:scale(1.2)}`（HTML 常见"轻微放大"反馈）
    xml18 = ('<?xml version="1.0"?><XWindow Width="360" Height="200">'
             '<XButton Name="tb" Text="btn" Width="120" Height="50"/>'
             '<style> #tb:hover { transform:scale(1.2) } #tb:focus { transform:scale(1.1) } </style>'
             '</XWindow>')
    winI, nI = load(xml18)
    check(winI.get_xml_value(nI['tb'], 'HoverTransform') == 'scale(1.2)',
          ':hover 的 transform 落到 HoverTransform',
          winI.get_xml_value(nI['tb'], 'HoverTransform'))
    check(winI.get_xml_value(nI['tb'], 'FocusTransform') == 'scale(1.1)',
          ':focus 的 transform 落到 FocusTransform',
          winI.get_xml_value(nI['tb'], 'FocusTransform'))
    check(winI.get_xml_value(nI['tb'], 'Transform') == '',
          '状态 transform 不落成常态（只在状态里生效）',
          winI.get_xml_value(nI['tb'], 'Transform'))
    winI.close()

    # ---- 状态化 box-shadow：`:hover{box-shadow:…}`（HTML "悬停抬起"的标配反馈）
    #      常态只给浅投影、悬停换成更厚的一层；投影不参与布局，兄弟不被挤动。
    xml19 = ('<?xml version="1.0"?><XWindow Width="360" Height="220">'
             '<XButton Name="sb" Text="card" Width="140" Height="60"'
             ' style="box-shadow:0 1px 2px rgba(0,0,0,0.2)"/>'
             '<style> #sb:hover { box-shadow:0 8px 20px rgba(0,0,0,0.55) }'
             ' #sb:focus { box-shadow:0 0 0 3px #1E90FF } </style>'
             '</XWindow>')
    winJ, nJ = load(xml19)
    check(winJ.get_xml_value(nJ['sb'], 'HoverShadow') == '0 8px 20px rgba(0,0,0,0.55)',
          ':hover 的 box-shadow 落到 HoverShadow',
          winJ.get_xml_value(nJ['sb'], 'HoverShadow'))
    check(winJ.get_xml_value(nJ['sb'], 'FocusShadow') == '0 0 0 3px #1E90FF',
          ':focus 的 box-shadow 落到 FocusShadow',
          winJ.get_xml_value(nJ['sb'], 'FocusShadow'))
    check(winJ.get_xml_value(nJ['sb'], 'BoxShadow') == '0 1px 2px rgba(0,0,0,0.2)',
          '常态 box-shadow 不被状态规则覆盖',
          winJ.get_xml_value(nJ['sb'], 'BoxShadow'))
    winJ.close()

    # ---- @keyframes：HTML 把关键帧写在块里、元素上只写 animation 名字；
    #      XML 早先只能把 Keyframes="0:#f00;…" 逐控件抄一遍。这里校验块能驱动真实动画。
    import time as _time
    from PIL import Image as _Image
    KF = ('@keyframes pulse{0%{background-color:#ff0000}50%{background-color:#00ff00}'
          '100%{background-color:#ff0000}}')

    def _maxdiff(img_a, img_b):
        from PIL import ImageChops as _Chops
        d = _Chops.difference(img_a.convert('RGB'), img_b.convert('RGB'))
        return max(max(v) for v in d.getextrema())

    def _frames(style, elem, tag):
        xml = ('<?xml version="1.0"?><XWindow Width="320" Height="180" Background="#202020">'
               + elem + '<style>' + style + '</style></XWindow>')
        win, names = load(xml)
        win.show()
        imgs = []
        for i in range(3):
            M.pump(120)
            png = os.path.join(LOG_DIR, f'_t_kf_{tag}{i}.png')
            win.save_png(png)
            imgs.append(_Image.open(png))
        win.close()
        return imgs

    btn = '<XButton id="b" Text="btn" Width="120" Height="60" X="20" Y="20"/>'
    mRule = _frames(KF + ' #b{background-color:#ff0000;animation:pulse 600ms linear infinite}',
                    btn, 'rule')
    check(_maxdiff(mRule[0], mRule[2]) > 60,
          '@keyframes 块驱动动画（帧间像素确实变色）', _maxdiff(mRule[0], mRule[2]))

    mInline = _frames(KF, '<XButton id="b" Text="btn" Width="120" Height="60" X="20" Y="20"'
                          ' style="background-color:#ff0000;'
                          'animation:pulse 600ms linear infinite"/>', 'inline')
    check(_maxdiff(mInline[0], mInline[2]) > 60,
          '行内 animation 也能引用 @keyframes', _maxdiff(mInline[0], mInline[2]))

    mUnknown = _frames(KF + ' #b{background-color:#ff0000;'
                      'animation:nope 600ms linear infinite}', btn, 'unknown')
    check(_maxdiff(mUnknown[0], mUnknown[2]) < 8,
          'animation 名字在表中不存在时不动（不瞎猜）', _maxdiff(mUnknown[0], mUnknown[2]))

    # ---- 容器（div）的 :hover：HTML 里列表行/卡片的标配反馈。
    # 容器是空 STATIC、被画面层盖住收不到鼠标消息，命中由宿主 OnMouseMove 判。
    xml20 = ('<XWindow Width="320" Height="200" Background="#FFFFFF">'
             '<style>#d:hover{background:#00FF00}</style>'
             '<div id="d" Left="10" Top="60" Width="200" Height="90" '
             'style="background:#123456"></div></XWindow>')
    winK, nK = load(xml20)
    winK.show()
    M.pump(200)
    from _kit import hwnd_of
    HK = hwnd_of(winK)
    import ctypes
    kids = []

    def _cb(hh, _l):
        kids.append(hh)
        return True

    import ctypes.wintypes as _wt
    _u = ctypes.windll.user32
    _u.EnumChildWindows(HK, ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.c_void_p,
                                              ctypes.c_void_p)(_cb), 0)
    _buf = ctypes.create_unicode_buffer(64)
    hostH = None
    for hh in kids:
        _u.GetClassNameW(ctypes.c_void_p(hh), _buf, 64)
        if _buf.value == 'PyMfcNativeWindow' and _u.GetDlgCtrlID(hh) == 0:
            hostH = hh
            break
    check(hostH is not None, '宿主窗口可定位（hover 注入前提）', hostH)

    def _shot(tag):
        png = os.path.join(LOG_DIR, f'_t_chv_{tag}.png')
        winK.save_png(png)
        return _Image.open(png).convert('RGB').getpixel((80, 130))

    base = _shot('base')
    _u.SendMessageW(ctypes.c_void_p(hostH), 0x0200, 0, (100 << 16) | 60)
    M.pump(300)
    inside = _shot('in')
    _u.SendMessageW(ctypes.c_void_p(hostH), 0x0200, 0, (20 << 16) | 280)
    M.pump(300)
    out = _shot('out')
    check(base == (18, 52, 86), 'div 常态底色是 style 的 #123456', base)
    check(inside == (0, 255, 0), 'div:hover{background} 命中时变 #00FF00', inside)
    check(out == (18, 52, 86), '光标移出容器后底色复原', out)
    # 被其他窗口遮挡时系统会立刻回一个 WM_MOUSELEAVE（登记 TME_LEAVE 的副产物），
    # 这种假离开不能把 hover 擦掉，否则遮挡场景下 :hover 永远闪烁。
    _u.SendMessageW(ctypes.c_void_p(hostH), 0x0200, 0, (100 << 16) | 60)
    M.pump(200)
    _u.SendMessageW(ctypes.c_void_p(hostH), 0x02A3, 0, 0)
    M.pump(250)
    still = _shot('stay')
    check(still == (0, 255, 0), '假 WM_MOUSELEAVE（窗口被遮挡）不清 hover', still)
    winK.close()
    return finish('t_css_class')


if __name__ == '__main__':
    sys.exit(main())
