# -*- coding: utf-8 -*-
"""demo_canvas_edit.py — DrawCanvas 图元点选 / 框选 / 删除（纯 Python 预览）。

交互约定（都是引擎侧实现的，不是脚本自己算的）：
  · 左键点中图元  → 选中它（后画的在上，先中谁）
  · 空白处拖拽    → 框选，与橡皮筋相交的图元全选中
  · 点空白（不拖）→ 取消选择
  · Delete 键     → 删除当前选中（画布需先被点过，拿到焦点）
  · 每次选择变化都会触发 onCanvasSelect 回调，界面上的状态条跟着更新
"""
import os
import tempfile

import PyMfcXml as p

CANVAS = 420
HEIGHT = 320

XML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml" Title="画布图元编辑" Width="480" Height="420">
  <XLayout x:Name="root">
    <XQCanvas x:Name="cv" Width="%d" Height="%d" Canvas.Left="28" Canvas.Top="28" />
    <XLabel x:Name="hint" Width="420" Height="22" Canvas.Left="28" Canvas.Top="362"
               Text="点图元选中 · 空白拖拽框选 · Delete 删除" />
  </XLayout>
</XWindow>''' % (CANVAS, HEIGHT)

CMDS = ('Clear,#313842;'
        'Rect,20,20,120,80,#3574C8;'
        'Ellipse,180,20,110,110,#2FA36B;'
        'RoundRect,320,30,80,60,10,#C8703C;'
        'Line,20,140,400,140,#8A94A3,2;'
        'Text,20,170,画布里的文字图元，也可以选中,13,#E6E6E6')

PATH = os.path.join(tempfile.mkdtemp(prefix='pymfc_cvedit_'), 'cve.xml')


def build():
    with open(PATH, 'w', encoding='utf-8') as f:
        f.write(XML)
    return PATH


def main():
    names = {}
    w = p.guiload(build(), names=names)
    w.show()
    p.pump(250)

    cv = names['cv']
    w.set_canvas_commands(cv, CMDS)
    w.set_canvas_editable(cv, True)
    hint = names['hint']

    def on_sel(*a):
        sel = w.get_canvas_selection(cv)
        n = w.get_canvas_item_count(cv)
        w.set_text(hint, '共 %d 个图元 · 选中 %s' % (n, sel if sel else '无'))

    w.on_canvas_select(cv, on_sel)
    on_sel()

    while True:
        p.pump(40)


if __name__ == '__main__':
    main()
