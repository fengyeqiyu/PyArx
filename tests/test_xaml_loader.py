# -*- coding: utf-8 -*-
"""PyMfcXaml.xaml 加载器接口自测：解析 / 遍历 / 布局 / 序列化 / 报错。"""
import os
import sys

import PyMfcXaml
from PyMfcXaml import xaml

ROOT = r'G:\PyArx'
fails = []
checks = 0


def check(name, cond, detail=''):
    global checks
    checks += 1
    if not cond:
        fails.append((name, detail))
        print('FAIL  ' + name + ('  ' + detail if detail else ''))
    else:
        print('ok    ' + name)


# 1. 解析示例文件
doc = xaml.load(os.path.join(ROOT, 'examples', 'grid_demo.xaml'))
root = doc.root
check('root tag', root.tag == 'Window', root.tag)
check('root Title 属性', root.get('Title') == 'Grid 布局演示', root.get('Title'))
check('children 非空', len(root.children) >= 1, str(len(root.children)))

# 2. 按名字定位 / 属性
btn = root.find('btnLeft')
check('find btnLeft', btn is not None and btn.tag == 'Button', str(btn))
check('Button Content', btn.get('Content') == '左侧按钮', btn.get('Content') if btn else '')
lst = root.find('listRight')
check('find listRight', lst is not None and lst.tag == 'ListBox')

# 3. findTag
grids = root.findTag('Grid')
check('findTag Grid', len(grids) == 1, str(len(grids)))

# 4. names 映射
names = doc.names()
check('names 数量', len(names) >= 4, str(names))

# 5. 布局
doc.layout(0, 0, 420, 300)
bx, by, bw, bh = btn.bounds
check('button bounds 有效', bw > 0 and bh > 0, f'{bx},{by},{bw},{bh}')
gx, gy, gw, gh = grids[0].bounds
check('grid 扣掉 Margin', (gw, gh) == (420 - 24, 300 - 24), f'{gw}x{gh}')

# 6. 序列化往返
text = doc.toXml()
check('toXml 含 Window', '<Window' in text and 'Grid' in text)
doc2 = xaml.loadString(text)
check('toXml 可回读', doc2.root.tag == 'Window' and doc2.root.find('btnLeft') is not None)

# 7. setProperty + apply
ok = doc.setProperty('btnLeft', 'Content', '已改名')
check('setProperty 返回', bool(ok))
doc.apply()
check('setProperty 生效', doc.root.find('btnLeft').get('Content') == '已改名',
      doc.root.find('btnLeft').get('Content'))

# 8. toDict
d = root.toDict()
check('toDict 结构', d['tag'] == 'Window' and isinstance(d['children'], list))

# 9. 注册标签
tags = xaml.tags()
for t in ('Button', 'TextBox', 'ComboBox', 'Slider', 'TabControl', 'CalendarCtrl'):
    check('tags 含 ' + t, t in tags)
check('tags 数量 >= 26', len(tags) >= 26, str(len(tags)))

# 10. 颜色工具
c = xaml.parseColor('#3366FF')
check('parseColor', c is not None, str(c))
check('colorToString', xaml.colorToString(c).upper() == '#3366FF', xaml.colorToString(c))

# 11. 缺省尺寸（control_catalog 未加载时可为 None，不算失败）
ds = xaml.defaultSize('Button')
print('info  defaultSize(Button) =', ds)

# 12. 报错路径
try:
    xaml.loadString('<Window><Button x:Name="a"/></Window>')
    check('缺 xmlns:x 应报错', False, '未报错')
except RuntimeError as e:
    check('缺 xmlns:x 报错文案', 'XML 解析失败' in str(e), str(e))

try:
    xaml.load(os.path.join(ROOT, 'examples', '__not_exist__.xaml'))
    check('文件不存在应报错', False, '未报错')
except RuntimeError as e:
    check('文件不存在报错', True, str(e)[:60])

# 汇总
print()
print(f'共 {checks} 项，失败 {len(fails)} 项')
if fails:
    for name, detail in fails:
        print('  - ' + name + ' :: ' + detail)
    sys.exit(1)
print('XAML LOADER TEST PASSED')
