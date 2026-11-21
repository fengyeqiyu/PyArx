# -*- coding: utf-8 -*-
"""demo_level_tab.py — 控件通用属性 Level（绘图层次）/ TabIndex（Tab 序号）。

XML 里两个属性都写在控件上，和 WPF 的 Panel.ZIndex / TabIndex 用法一致：
  Level="2"   越大越靠上（后画，压住小的）
  TabIndex="1" 只决定 Tab 键的先后，跟谁压着谁无关

界面：
  · 左上两块半重叠的色块，演示 Level 决定谁压谁；
  · 下方三个输入框，TabIndex 故意打乱声明顺序，Tab 走的是序号不是声明顺序；
  · 右侧按钮：置顶 / 置底 / 交换，实时改 Level 并刷新状态条。
"""
import os
import tempfile

import PyMfcXml as p

XML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml" Title="层次与 Tab 序号" Width="560" Height="440">
  <XLayout x:Name="root">
    <XLabel x:Name="a" Width="170" Height="90" Canvas.Left="30" Canvas.Top="30"
               Text="A 块" Background="#C0392B" Foreground="#FFFFFF" Level="0" />
    <XLabel x:Name="b" Width="170" Height="90" Canvas.Left="70" Canvas.Top="60"
               Text="B 块" Background="#2E86C1" Foreground="#FFFFFF" Level="1" />

    <XTextField x:Name="e1" Width="150" Height="30" Canvas.Left="30" Canvas.Top="190" TabIndex="3" Text="第一个声明，Tab 第 3 个" />
    <XTextField x:Name="e2" Width="150" Height="30" Canvas.Left="30" Canvas.Top="230" TabIndex="1" Text="第二个声明，Tab 第 1 个" />
    <XTextField x:Name="e3" Width="150" Height="30" Canvas.Left="30" Canvas.Top="270" TabIndex="2" Text="第三个声明，Tab 第 2 个" />

    <XButton x:Name="btnTop" Text="B 置顶" Width="110" Height="34" Canvas.Left="250" Canvas.Top="190" TabIndex="4" />
    <XButton x:Name="btnBottom" Text="B 置底" Width="110" Height="34" Canvas.Left="250" Canvas.Top="234" TabIndex="5" />
    <XButton x:Name="btnSwap" Text="交换 A / B" Width="110" Height="34" Canvas.Left="250" Canvas.Top="278" TabIndex="6" />
    <XLabel x:Name="status" Width="480" Height="26" Canvas.Left="30" Canvas.Top="340" Text="" />
  </XLayout>
</XWindow>'''


def build():
    path = os.path.join(tempfile.mkdtemp(prefix='pymfc_leveltab_'), 'leveltab.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XML)
    return path


def main():
    names = {}
    w = p.guiload(build(), names=names)
    w.show()
    p.pump(250)

    a, b, status = names['a'], names['b'], names['status']

    def refresh(*_):
        w.set_text(status, 'A.Level=%d  B.Level=%d ｜ Tab 序：e2(1) → e3(2) → e1(3)'
                   % (w.get_level(a), w.get_level(b)))

    def on_top(*_):
        w.bring_to_front(b)
        w.set_text(status, 'B 已置顶（Level=%d）' % w.get_level(b))

    def on_bottom(*_):
        w.send_to_back(b)
        w.set_text(status, 'B 已置底（Level=%d）' % w.get_level(b))

    def on_swap(*_):
        la, lb = w.get_level(a), w.get_level(b)
        w.set_level(a, lb)
        w.set_level(b, la)
        w.set_text(status, '交换后 A.Level=%d  B.Level=%d' % (w.get_level(a), w.get_level(b)))

    w.on_click(names['btnTop'], on_top)
    w.on_click(names['btnBottom'], on_bottom)
    w.on_click(names['btnSwap'], on_swap)
    refresh()

    p.pump(1200)


if __name__ == '__main__':
    main()
