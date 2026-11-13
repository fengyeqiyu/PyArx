# -*- coding: utf-8 -*-
"""第八批演示：StackView 三页切换（页内容是真子控件，整组平移），窗口保持打开。"""
import os, sys, tempfile
import PyMfcXaml as p

XAML = '''<?xml version="1.0" encoding="utf-8"?>
<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="页容器演示" Width="460" Height="330">
  <Canvas>
    <StackView x:Name="sv" Width="360" Height="240" Canvas.Left="20" Canvas.Top="16" />
    <Button x:Name="b1" Text="第一页：按钮" Width="360" Height="214" Canvas.Left="20" Canvas.Top="16" />
    <TextBlock x:Name="t2" Text="第二页：文字页" Width="360" Height="214" Canvas.Left="20" Canvas.Top="16" />
    <TableView x:Name="tv3" Width="360" Height="214" Canvas.Left="20" Canvas.Top="16" />
  </Canvas>
</Window>'''

path = os.path.join(tempfile.mkdtemp(prefix='pymfc_pgdemo_'), 'pg.xaml')
open(path, 'w', encoding='utf-8').write(XAML)
names = {}
w = p.guiload(path, names=names)
w.show()
p.pump(300)
sv = names['sv']
w.addPage(sv, [names['b1']])
w.addPage(sv, [names['t2']])
w.addPage(sv, [names['tv3']])
w.setTableColumns(names['tv3'], '零件	数量	单价')
for row in ('接头	12	3.50', '法兰	8	12.00', '垫片	40	0.80'):
    w.addTableRow(names['tv3'], row)
w.setTableSelectedRow(names['tv3'], 1)
idx = int(sys.argv[1]) if len(sys.argv) > 1 else 1
if idx:
    w.setViewIndex(sv, idx)
    p.pump(400)
print('pages=%d index=%d' % (w.getViewCount(sv), w.getViewIndex(sv)), flush=True)
p.pump(300)
while True:
    p.pump(50)
