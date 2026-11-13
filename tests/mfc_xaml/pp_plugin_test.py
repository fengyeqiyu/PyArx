# -*- coding: utf-8 -*-
"""xamlload 对象图 + c.Name.click = fn 绑定 + XAML 定位 + 批量灌数据。"""
import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, pp_plugin

PASS=[0]; FAIL=[0]
def ck(n, c, d=''):
    if c: PASS[0]+=1; print('PASS %s %s'%(n,d), flush=True)
    else: FAIL[0]+=1; print('FAIL %s %s'%(n,d), flush=True)

class App:
    def __init__(self, w): self.win = w

win = M.Window('attr-test', 700, 460); win.show(); M.pump(250); win.setTheme(False)
app = App(win)

c = pp_plugin.xamlload(r'G:\PyArx\plugins\side_panel.xaml', app=app, x=10, y=10, w=640, h=380)
M.pump(120)

ck('对象图：Attr 取到子控件', c.BtnApply.name == 'BtnApply', repr(c.BtnApply))
ck('XAML 定位', c.BtnApply.rect[2] > 190 and c.Table.rect[2] == 640 and c.Table.rect[3] == 306, str(c.Table.rect))

# 用户要的写法
def on_apply(comp, name):
    hits.append(name)
hits = []
c.BtnApply.click = on_apply
ck('c.BtnApply.click = fn 已绑上', bool(c.BtnApply.click), str(c.BtnApply.click))

# XAML 里声明的 on_click 仍在
ck('XAML 声明的 on_click', c._decl.get('BtnLoad',{}).get('click') == 'on_load', str(c._decl.get('BtnLoad')))

rows = [[u'AT1-%d'%(i+1), u'变压器', u'AT1', u'YJV22-4x35', u'630', u'10kV', u'12'] for i in range(39)]
c.set_rows('Table', rows); M.pump(50)
ck('一次灌入 39 行', win.getTableRowCount(c.Table.id) == 39, str(win.getTableRowCount(c.Table.id)))

print('=== %d/%d PASS ==='%(PASS[0], PASS[0]+FAIL[0]), flush=True)
os._exit(0)
