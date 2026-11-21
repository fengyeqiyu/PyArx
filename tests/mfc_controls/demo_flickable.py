# -*- coding: utf-8 -*-
"""Flickable 演示：可拖拽内容视口（1:1 跟随 + 松手惯性 + 触边弹性回弹）。

内容是真子窗，宿主把它们的父窗改成视口窗，于是超出视口的部分被父窗裁掉。
用法：先跑本脚本看窗口，再用 _verify_flickable.py 做数值/像素验收。
"""
import os
import tempfile

import PyMfcXml as p

VIEW_W, VIEW_H = 380, 220
ROW_H = 52
ROWS = ['条目 %02d · 内容 %d' % (i + 1, i + 1) for i in range(8)]
CONTENT_H = len(ROWS) * ROW_H          # 416 > 220，可上下拖

kids_xml = []
for i, t in enumerate(ROWS):
    kids_xml.append(
        '<XLabel x:Name="r%d" Text="%s" Width="340" Height="40" '
        'Background="#2B313C" Foreground="#E6EDF3" '
        'Canvas.Left="30" Canvas.Top="%d" />' % (i, t, 46 + i * ROW_H))

XML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml" Title="Flickable 拖拽演示" Width="420" Height="382">
  <XCanvas>
    <XLabel x:Name="lb" Text="拖住内容上下拖，松手会继续滑，拖过头会弹回" Width="386" Height="24" Canvas.Left="18" Canvas.Top="14" />
    <XFlickable x:Name="fl" Width="%d" Height="%d" Canvas.Left="20" Canvas.Top="46" />
    %s
    <XLabel x:Name="hint" Text="offset: 0, 0" Width="380" Height="22" Canvas.Left="20" Canvas.Top="288" />
  </XCanvas>
</XWindow>''' % (VIEW_W, VIEW_H, '\n    '.join(kids_xml))


def build():
    path = os.path.join(tempfile.mkdtemp(prefix='pymfc_flick_'), 'flick.xml')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(XML)
    return path


if __name__ == '__main__':
    path = build()
    names = {}
    w = p.guiload(path, names=names)
    w.show()
    p.pump(300)
    fid = names['fl']
    kids = [names['r%d' % i] for i in range(len(ROWS))]
    w.set_flick_content(fid, kids, 340, CONTENT_H)
    p.pump(150)
    print('flick id=%d kids=%s offset=%d,%d content=%dx%d'
          % (fid, kids, w.get_flick_offset_x(fid), w.get_flick_offset_y(fid), 340, CONTENT_H),
          flush=True)
    # 状态条跟手刷新：窗口再放大也一眼能看出当前偏移，不至于被窗口下边界吃掉
    last = None
    while True:
        cur = 'offset: %d, %d   content: 340x%d' % (w.get_flick_offset_x(fid),
                                                     w.get_flick_offset_y(fid), CONTENT_H)
        if cur != last:
            w.set_text(names['hint'], cur)
            last = cur
        p.pump(40)
