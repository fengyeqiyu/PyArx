# -*- coding: utf-8 -*-
"""最小实验：ListCtrl + addColumn + addTableRow 是否真能出数据行。"""
import os, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release')
os.add_dll_directory(r'G:\PyArx\X64\Release')
import PyMfcXml as M

win = M.Window('table probe', 800, 500)
win.set_theme(False)
win.add_list_ctrl(4501, 10, 10, 700, 400)
win.add_column(4501, '编号', 100)
win.add_column(4501, '名称', 200)
win.add_column(4501, '上游', 100)
win.add_table_row(4501, 'TR1\t变压器\t')
win.add_table_row(4501, 'AT1\t配电柜\tTR1')
win.add_table_row(4501, 'AT2\t配电柜\tTR1')
print('rows =', win.get_table_row_count(4501))
win.show()
time.sleep(0.4)
M.pump(30)
win.save_png(r'G:\PyArx\docs\_pp_probe_table.png')
print('截图', os.path.getsize(r'G:\PyArx\docs\_pp_probe_table.png'))
# 再看 setTableCell 是否可写
print('cell(1,1) =', repr(win.get_table_cell(4501, 1, 1)))
win.close()
