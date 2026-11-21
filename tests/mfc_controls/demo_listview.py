# -*- coding: utf-8 -*-
"""第七批演示：虚拟化列表（21 行只画可见段）+ 惯性滚动。窗口保持打开。"""
import os, tempfile
import PyMfcXml as p

ROWS = ';'.join(['工程 %02d|负责：张%d|%d%%' % (i + 1, (i % 9) + 1, (i * 7) % 100) for i in range(21)])

XML = '''<?xml version="1.0" encoding="utf-8"?>
<XWindow xmlns:x="http://schemas.microsoft.com/winfx/2006/xml" Title="虚拟化列表演示" Width="480" Height="400">
  <XCanvas>
    <XLabel x:Name="lb" Text="VirtualList：21 行，只画可见的 5 行" Width="360" Height="26" Canvas.Left="14" Canvas.Top="8" />
    <XVirtualList x:Name="lv" Items="%s" ItemHeight="44" Width="340" Height="220" Canvas.Left="14" Canvas.Top="44" />
    <XSwitch x:Name="sw" Width="56" Height="28" Canvas.Left="380" Canvas.Top="44" Checked="true" />
  </XCanvas>
</XWindow>''' % ROWS

path = os.path.join(tempfile.mkdtemp(prefix='pymfc_lvdemo_'), 'lv.xml')
open(path, 'w', encoding='utf-8').write(XML)
names = {}
w = p.guiload(path, names=names)
w.show()
p.pump(300)
lv = names['lv']
w.set_list_selected(lv, 0)
p.pump(120)
print('count=%d sel=%d scroll=%d' % (w.get_list_item_count(lv), w.get_list_selected(lv),
                                     w.get_list_scroll(lv)), flush=True)
while True:
    p.pump(50)
