import io, os
d = r'G:\PyArx\PyMfcXaml\parts'

def split(src, ranges, tag):
    p = os.path.join(d, src)
    lines = io.open(p, encoding='utf-8').read().split('\n')
    made = []
    for name, a, b, desc in ranges:
        body = '\n'.join(lines[a - 1:b])
        out = os.path.join(d, name)
        io.open(out, 'w', encoding='utf-8', newline='').write(
            '// %s  （%s 行 %d-%d）\n%s\n' % (desc, os.path.basename(src), a, b, body))
        made.append((name, b - a + 1, desc))
    os.remove(p)
    for m in made:
        print('%-26s %5d 行  %s' % m)

split('02-host.inc', [
    ('02a-host-core.inc',   1,    1166, 'PyMfcHost：类声明/构造/窗口与子控件管理/选项卡页/自绘过程/图片/Tab 焦点'),
    ('02b-host-widgets.inc', 1167, 2609, 'PyMfcHost：PyMfcSkinBox 自绘小控件（Switch/RangeSlider/SpinBox/Spin/Tumbler/ScrollBar/Dial）'),
    ('02c-host-fill.inc',   2610, 3443, 'PyMfcHost：内容填充（Tab/Tree/ListCtrl/Grid/ToolBar）'),
], '02')

split('03-procs.inc', [
    ('03a-procs-skin.inc',   1,    1795, '窗口过程：全控件自绘 + 皮肤绘制（owner-draw 过程体）'),
    ('03b-procs-canvas.inc', 1796, 2120, '窗口过程：配电画布鼠标状态机 + 可编辑表格 Grid 过程'),
    ('03c-procs-off.inc',    2121, 2327, '离屏渲染：BGRA→PNG(WIC) / D2D 配电图 / nanosvg SVG'),
], '03')
