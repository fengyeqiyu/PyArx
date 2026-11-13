import os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp, pp_events as E
app = ppApp.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
tbl = E.find_control(app.win.hwnd, ppApp.TBL)
rc = app.win.cellRect(ppApp.TBL, 5, 3)
E.double_click(tbl, rc[0]+rc[2]//2, rc[1]+rc[3]//2); M.pump(200)
ce = app._ce
print('ce', ce, '原 cable', repr(app.nodes[5]['cable']))
app.win.setText(ce[2], 'YJV22-4x70'); M.pump(80)
print('EDIT 文本', repr(app.win.getText(ce[2])))
rc4 = app.win.cellRect(ppApp.TBL, 6, 3)
E.click(tbl, rc4[0]+rc4[2]//2, rc4[1]+rc4[3]//2); M.pump(250)
print('ce 之后', app._ce, 'cable 之后', repr(app.nodes[5]['cable']))
print('表格格', repr(app.win.getTableCell(ppApp.TBL, 5, 3)))
os._exit(0)
