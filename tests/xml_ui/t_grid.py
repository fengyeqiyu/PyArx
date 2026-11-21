# -*- coding: utf-8 -*-
"""CSS Grid：HTML 写法（display:grid + grid-template-* + grid-column/grid-row）与
XAML 的 <XGrid> + Grid.Row/Grid.Column 必须给出同一套矩形。

用户口径：设计基准是 HTML/CSS，不是 QML；HTML 能写的网格语义 XML 侧必须同样能写。
"""
import os
import sys

BIN = os.environ.get('PYARX_BIN') or r'G:\PyArx\X64\Release'
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M  # noqa: E402

ok = 0
fail = 0


def check(name, cond, extra=''):
    global ok, fail
    if cond:
        ok += 1
        print('PASS', name)
    else:
        fail += 1
        print('FAIL', name, extra)


def layout(xml, w, h, tag):
    path = os.path.join(r'G:\PyArx\log', '_t_grid_%s.xml' % tag)
    with open(path, 'w', encoding='utf-8-sig') as f:
        f.write(xml)
    return M.xml_layout(path, w, h)


# 1) fr 权重 + 跨列：1fr/2fr 两列，第三项跨两列
r = layout('<window Width="420" Height="220">'
           '<div Name="g" style="display:grid;grid-template-columns:1fr 2fr;'
           'grid-template-rows:32px 1fr;width:400;height:200">'
           '<label Name="a" text="A" style="grid-column:1;grid-row:1"/>'
           '<label Name="b" text="B" style="grid-column:2;grid-row:1"/>'
           '<label Name="c" text="C" style="grid-column:1 / span 2;grid-row:2"/>'
           '</div></window>', 420, 220, 'fr')
check('1fr 列宽 = 100', r['a'][2] == 133, r['a'])
check('2fr 列宽 = 267', r['b'][2] == 267 and r['b'][0] == 133, r['b'])
check('grid-column 跨两列 = 全宽', r['c'][2] == 400, r['c'])
check('grid-row:2 落到第二行', r['c'][1] == 32 and r['c'][3] == 168, r['c'])
check('格子里的项默认拉伸（不是文字宽）', r['a'][2] > 48 and r['a'][3] == 32, r['a'])

# 2) auto-placement：没写落格的项按源顺序行优先填格
r2 = layout('<window Width="320" Height="120">'
            '<div Name="g" style="display:grid;grid-template-columns:1fr 1fr;'
            'grid-template-rows:40px 40px;width:300;height:80">'
            '<label Name="p" text="P"/><label Name="q" text="Q"/><label Name="r" text="R"/>'
            '</div></window>', 320, 120, 'auto')
check('auto: 第 1 项 (0,0)', (r2['p'][0], r2['p'][1], r2['p'][2]) == (0, 0, 150), r2['p'])
check('auto: 第 2 项换到第 2 列', (r2['q'][0], r2['q'][1]) == (150, 0), r2['q'])
check('auto: 第 3 项回到下一行', (r2['r'][0], r2['r'][1]) == (0, 40), r2['r'])

# 3) 轨道可用空间按 content box（扣 padding），与 XAML <XGrid> 同口径
xml_html = ('<window Width="300" Height="200">'
            '<div Name="g" style="display:grid;grid-template-columns:1fr 1fr;'
            'grid-template-rows:1fr;width:240;height:120;padding:12">'
            '<label Name="m" text="M" style="grid-column:1"/>'
            '<label Name="n" text="N" style="grid-column:2"/>'
            '</div></window>')
xml_xaml = ('<window Width="300" Height="200">'
            '<XGrid Name="g" Width="240" Height="120" Padding="12">'
            '<XGrid.ColumnDefinitions><ColumnDefinition Width="*"/><ColumnDefinition Width="*"/>'
            '</XGrid.ColumnDefinitions>'
            '<XGrid.RowDefinitions><RowDefinition Height="*"/></XGrid.RowDefinitions>'
            '<label Name="m" text="M" Grid.Column="0"/>'
            '<label Name="n" text="N" Grid.Column="1"/>'
            '</XGrid></window>')
h = layout(xml_html, 300, 200, 'pad_html')
x = layout(xml_xaml, 300, 200, 'pad_xaml')
check('HTML 写法扣 padding 分列', h['m'][2] == 108 and h['n'][0] == 12 + 108, repr((h['m'], h['n'])))
check('XAML <XGrid> 同口径', x['m'][2] == 108 and x['n'][0] == 12 + 108, repr((x['m'], x['n'])))

# 4) grid-template-rows 的 1fr 吃掉剩余高度（32 + 1fr + 32）
r4 = layout('<window Width="300" Height="200">'
            '<div Name="g" style="display:grid;grid-template-rows:32px 1fr 32px;'
            'grid-template-columns:1fr;width:260;height:160">'
            '<label Name="t" text="T" style="grid-row:1"/>'
            '<label Name="m" text="M" style="grid-row:2"/>'
            '<label Name="b" text="B" style="grid-row:3"/>'
            '</div></window>', 300, 200, 'rows')
check('row1 固定 32', r4['t'][1] == 0 and r4['t'][3] == 32, r4['t'])
check('row2 吃掉剩余 96', r4['m'][1] == 32 and r4['m'][3] == 96, r4['m'])
check('row3 固定 32 且贴底', r4['b'][1] == 128 and r4['b'][3] == 32, r4['b'])

# 5) 原生 <button>/<input> 的伪尺寸不能把格子撑住：CSS grid 的项默认拉伸两轴
r5 = layout('<window Width="420" Height="240">'
            '<div Name="g" style="display:grid;grid-template-columns:1fr 1fr;'
            'grid-template-rows:40px 40px 1fr;width:400;height:200">'
            '<button Name="btn" text="OK" style="grid-column:1;grid-row:1"/>'
            '<input Name="inp" type="text" value="abc" style="grid-column:2;grid-row:1"/>'
            '<button Name="wide" text="W" style="grid-column:1 / span 2;grid-row:3"/>'
            '</div></window>', 420, 240, 'btncell')
check('button 拉满单元格宽高', (r5['btn'][2], r5['btn'][3]) == (200, 40), r5['btn'])
check('input 拉满单元格宽高', (r5['inp'][0], r5['inp'][2], r5['inp'][3]) == (200, 200, 40), r5['inp'])
check('跨两列的 button 拉满 400 宽', (r5['wide'][2], r5['wide'][3]) == (400, 120), r5['wide'])
print('t_grid: PASS=%d FAIL=%d' % (ok, fail))
sys.exit(0 if fail == 0 else 1)
