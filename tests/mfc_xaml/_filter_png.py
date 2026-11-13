# -*- coding: utf-8 -*-
"""验证表头漏斗筛选：直接调 tableSetFilter 后保存窗口截图（不依赖屏幕抓取）。"""
import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXaml as M
import pp_app

path = r'G:\PyArx\docs\_big_20_10_50_allstart.csv'
text = open(path, encoding='utf-8').read()
app = pp_app.App()
app.win.show()
M.pump(120)
app.showEditor()
M.pump(150)
app.importCsvText(text, os.path.basename(path))
M.pump(400)
app.win.savePng(r'G:\PyArx\docs\_f_before.png')

M.pump(2400)   # 重新截图前先让画面稳定
app.win.tableSetFilter(3021, 0, 'GRID')
M.pump(600)
app.win.savePng(r'G:\PyArx\docs\_f_after.png')
print('filtered done', flush=True)
