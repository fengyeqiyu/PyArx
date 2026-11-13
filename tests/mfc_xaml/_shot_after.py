# -*- coding: utf-8 -*-
"""改后观感取证：编辑侧板（白卡圆角 + 分割条）、详情卡（阴影 + 圆角）两张。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
import PyMfcXaml as M
import pp_app

app = pp_app.App(); app.win.show(); M.pump(80)
app.showEditor(); M.pump(200)
app.win.savePng(r'G:\PyArx\docs\_after_editor.png')

app.fillDetail(app.nodes[1]['code']); M.pump(120)
app.win.savePng(r'G:\PyArx\docs\_after_detail.png')
print('shots saved', flush=True)
os._exit(0)
