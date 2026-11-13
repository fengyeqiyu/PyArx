import io, os
B = r'G:\PyArx\docs\_backup_PyMfcXaml.cpp'
blines = io.open(B, encoding='utf-8-sig').read().split('\n')
host = blines[443:6262]

d = r'G:\PyArx\PyMfcXaml\parts'
for f in ('02a-host-core.inc', '02b-host-widgets.inc', '02c-host-fill.inc'):
    p = os.path.join(d, f)
    if os.path.exists(p):
        os.remove(p)

ranges = [
    ('02a-host-core.inc',    1,    1165, 'PyMfcHost：类声明/构造/窗口与子控件管理/选项卡页/自绘过程/图片/Tab 焦点'),
    ('02b-host-widgets.inc', 1166, 2608, 'PyMfcHost：PyMfcSkinBox 自绘小控件（Switch/RangeSlider/SpinBox/Spin/Tumbler/ScrollBar/Dial）'),
    ('02c-host-fill.inc',    2609, 3442, 'PyMfcHost：内容填充（Tab/Tree/ListCtrl/Grid/ToolBar）'),
    ('02d-host-export.inc',  3443, 3778, 'PyMfcHost：控件级导出（PrintWindow/WIC PNG）与 Z 序 Level'),
    ('02e-host-canvas.inc',  3779, 4609, 'PyMfcHost：画布图元解析/选中/删除、透明度、页导航动画'),
    ('02f-host-anim.inc',    4610, 5155, 'PyMfcHost：属性动画引擎（Number/Color/PropertyAnimation）'),
    ('02g-host-props.inc',   5156, len(host), 'PyMfcHost：通用基础属性推送/滚轮路由/当前活动单元格/富文本'),
]
for name, a, b, desc in ranges:
    body = '\n'.join(host[a - 1:b])
    io.open(os.path.join(d, name), 'w', encoding='utf-8', newline='').write(
        '// %s  （02-host 原 %d-%d）\n%s\n' % (desc, a, b, body))
    print('%-24s %5d 行  %s' % (name, b - a + 1, desc))
print('总覆盖 %d 行 / 原 %d 行' % (sum(r[2]-r[1]+1 for r in ranges), len(host)))
