import ctypes, ctypes.wintypes as wt, os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp, pp_events as E
u = ctypes.WinDLL('user32', use_last_error=True)
u.GetFocus.restype=wt.HWND; u.GetDlgCtrlID.argtypes=[wt.HWND]; u.GetDlgCtrlID.restype=ctypes.c_int
app = ppApp.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
print('onBlur 绑定?', hasattr(app.win,'onBlur'))
tbl = E.find_control(app.win.hwnd, ppApp.TBL)
rc = app.win.cellRect(ppApp.TBL, 5, 1)
E.double_click(tbl, rc[0]+rc[2]//2, rc[1]+rc[3]//2); M.pump(200)
eh = E.find_control(app.win.hwnd, app._ce[2])
print('编辑中 focus id =', u.GetDlgCtrlID(u.GetFocus() or 0))
rc4 = app.win.cellRect(ppApp.TBL, 6, 3)
E.click(tbl, rc4[0]+rc4[2]//2, rc4[1]+rc4[3]//2); M.pump(220)
print('点另一行后 focus id =', u.GetDlgCtrlID(u.GetFocus() or 0), ' ce =', app._ce)
print('EDIT 可见?', bool(u.IsWindowVisible(eh)))
os._exit(0)
