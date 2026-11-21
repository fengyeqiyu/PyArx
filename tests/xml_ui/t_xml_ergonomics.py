# -*- coding: utf-8 -*-
"""XML 本身像 HTML 一样省事：文本即内容、Name 可省、根可省、缺省尺寸自动。

语言仍然是 XML（不是换成 HTML 标签），只是把"必须手写坐标 / 必须命名 / 必须包 Window"
这些硬要求去掉。所有断言都读真实控件（文本 / 尺寸 / 客户区），不做影子计算。
"""
import ctypes
import sys
from ctypes import wintypes

from _kit import M, check, finish


u = ctypes.WinDLL('user32')
u.GetClientRect.argtypes = [wintypes.HWND, ctypes.c_void_p]


def client(win):
    r = wintypes.RECT()
    u.GetClientRect(win.hwnd, ctypes.byref(r))
    return r.right, r.bottom


def main():
    # 1) 文本即内容 + Name 可省 + 根可省（容器当根）
    names = {}
    win = M.guiload_text(
        '<XStackPanel Padding="12" Spacing="8">'
        '<XLabel>标题</XLabel>'
        '<XStackPanel Orientation="Horizontal" Spacing="8">'
        '<XTextField Placeholder="名称"/>'
        '<XButton>确定</XButton>'
        '</XStackPanel>'
        '<XCheckBox>启用</XCheckBox>'
        '</XStackPanel>', context=globals(), names=names)
    win.show()
    M.pump(120)

    check(len(names) == 4, '容器当根：自动包窗口并建出全部控件', names)
    check(win.get_xml_value(names['label_1000'], 'text') == '标题', '文本即内容 -> Text',
          win.get_xml_value(names['label_1000'], 'text'))
    check(win.get_xml_value(names['button_1002'], 'text') == '确定', '按钮文本即内容')
    check(win.get_xml_value(names['checkbox_1003'], 'text') == '启用', '复选文本即内容')

    cw, ch = client(win)
    check(cw == 320, '片段根默认内容宽 320（外框已折算标题栏）', (cw, ch))
    # HTML 自然尺寸语义：按钮自然高 30（XmlCore.cpp 默认值表），横排行高取 max(输入26, 按钮30)=30
    check(ch == 118, '内容高自动算出 = 12+24+8+30+8+24+12', (cw, ch))

    # 2) 块级撑满 / 行内自然宽
    lw, _ = win.get_size(names['label_1000'])
    tfw, _ = win.get_size(names['textfield_1001'])
    bw, _ = win.get_size(names['button_1002'])
    check(lw == cw - 24, '竖排控件撑满内容宽', (lw, cw - 24))
    # HTML 默认尺寸表：老写法 XTextField 160 宽 / 26 高；按钮自然宽 88（内容宽兜底，XmlCore.cpp 默认值表）
    check(tfw == 160 and bw == 88, '横排按自然宽（输入 160 / 按钮 88）', (tfw, bw))
    check(tfw + bw <= cw - 24 - 8, '横排两项宽度 + 间距不溢出', (tfw, bw, cw))
    win.close()

    # 3) 单个控件当根
    names2 = {}
    w2 = M.guiload_text('<XButton>确定</XButton>', context=globals(), names=names2)
    w2.show()
    M.pump(80)
    check(len(names2) == 1 and w2.get_xml_value(names2['button_1000'], 'text') == '确定',
          '单个控件当根也能建', names2)
    # 单控件根高 = 按钮自然高 30（XmlCore.cpp 默认值表）；客户区按真实非客户区校准后等于内容高
    check(client(w2) == (320, 30), '单控件根：高按控件缺省', client(w2))
    w2.close()

    # 4) 显式写法与内容写法等价
    n3, n4 = {}, {}
    w3 = M.guiload_text('<XWindow Width="300" Height="120">'
                        '<XLabel Name="L" Text="标题" Left="12" Top="12" Width="200"/></XWindow>',
                        context=globals(), names=n3)
    w4 = M.guiload_text('<XWindow Width="300" Height="120">'
                        '<XLabel Name="L">标题</XLabel></XWindow>', context=globals(), names=n4)
    w3.show(); w4.show(); M.pump(80)
    check(w3.get_xml_value(n3['L'], 'text') == w4.get_xml_value(n4['L'], 'text') == '标题',
          'Text="…" 与 <XLabel>…</XLabel> 等价')
    w3.close(); w4.close()

    # 5) 显式 <XWindow> 尺寸语义没被改（仍是外框尺寸，不是片段根那套折算）
    n5 = {}
    w5 = M.guiload_text('<XWindow Width="420" Height="320">'
                        '<XStackPanel Padding="12"><XLabel Name="T">x</XLabel></XStackPanel>'
                        '</XWindow>', context=globals(), names=n5)
    w5.show(); M.pump(80)
    cw5, ch5 = client(w5)
    tw5, _ = w5.get_size(n5['T'])
    check(cw5 < 420 and tw5 == cw5 - 24, 'XWindow 尺寸语义保持原样（外框 420，标签按内容区撑满）',
          (cw5, tw5))
    w5.close()

    # 6) 片段根的开箱默认：没写 Spacing/Padding 时 8 / 12
    n6 = {}
    w6 = M.guiload_text('<XStackPanel><XLabel>a</XLabel><XLabel>b</XLabel></XStackPanel>',
                        context=globals(), names=n6)
    w6.show(); M.pump(80)
    cw6, ch6 = client(w6)
    lw6, _ = w6.get_size(n6['label_1000'])
    check(cw6 == 320 and ch6 == 12 + 24 + 8 + 24 + 12,
          '片段根默认 padding 12 / spacing 8', (cw6, ch6))
    check(lw6 == cw6 - 24, '默认内边距下仍撑满内容宽', (lw6, cw6 - 24))
    w6.close()

    return finish('t_xml_ergonomics')


if __name__ == '__main__':
    sys.exit(main())
