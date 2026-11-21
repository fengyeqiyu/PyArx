# -*- coding: utf-8 -*-
"""运行机制框架验证：注册表 / 建实例 / XML 定位 / 回调绑定 / 批量灌数据。"""
import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xml')
sys.path.insert(0, r'G:\PyArx')

from MfcXmlRuntime import Runtime, xmlload, available
import pp_plugin            # 兼容转发是否还通

PASS=[0]; FAIL=[0]
def ck(n, c, d=''):
    if c: PASS[0]+=1; print('PASS %s %s'%(n,d), flush=True)
    else: FAIL[0]+=1; print('FAIL %s %s'%(n,d), flush=True)

ck('插件目录可枚举', 'side_panel.xml' in available(), str(available()))
ck('注册表列出 side_panel', 'side_panel' in Runtime.list(), str(Runtime.list()))
ck('兼容转发 pp_plugin 仍可用', hasattr(pp_plugin, 'xmlload'), '')

import PyMfcXml as M
class App:
    def __init__(self, w): self.win = w

win = M.Window('runtime-test', 700, 460); win.show(); M.pump(250); win.set_theme(False)
app = App(win)

c = Runtime.create('side_panel', app, 10, 10, 640, 380)
M.pump(120)
ck('Runtime.create 出对象', c.BtnApply.name == 'BtnApply', repr(c.BtnApply))
ck('XML 定位', c.Table.rect == (10, 50, 640, 306), str(c.Table.rect))

hits=[]
def on_click(comp, name): hits.append(name)
c.BtnApply.click = on_click
ck('child.click 绑定', bool(c.BtnApply.click), str(c.BtnApply.click))
ck('XML 声明 on_click 仍在', c._decl.get('BtnLoad',{}).get('click')=='on_load', str(c._decl.get('BtnLoad')))

rows=[[u'AT1-%d'%(i+1),u'变压器',u'AT1',u'YJV22-4x35',u'630',u'10kV',u'12'] for i in range(39)]
c.set_rows('Table', rows); M.pump(50)
ck('批量灌 39 行', win.get_table_row_count(c.Table.id)==39, str(win.get_table_row_count(c.Table.id)))
ck('重排外框', (c.move(20,20,500,300) or True) and c.Table.rect[3]==226, str(c.Table.rect))

print('=== %d/%d PASS ==='%(PASS[0], PASS[0]+FAIL[0]), flush=True)
os._exit(0)
