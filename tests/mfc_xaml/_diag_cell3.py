import ctypes, ctypes.wintypes as wt, os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp, pp_events as E
u = ctypes.WinDLL('user32', use_last_error=True)
u.GetWindowRect.argtypes=[wt.HWND, ctypes.POINTER(wt.RECT)]
def pos(h):
    r=wt.RECT(); u.GetWindowRect(h, ctypes.byref(r)); return (r.left, r.top, r.right-r.left, r.bottom-r.top)
app = ppApp.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
eh = E.find_control(app.win.hwnd, ppApp.E_F0+1)
print('起始 EDIT', pos(eh), 'tbl_pos', app._tblPos)
app.win.moveMany([(ppApp.E_F0+1, 694, 251, 98, 21, True)]); M.pump(120)
print('手动 moveMany 到 (694,251) 后', pos(eh))
app._cellBegin(5, 1); M.pump(150)
print('_cellBegin 之后', pos(eh), 'ce', app._ce)
app._layout(); M.pump(120)
print('再调 _layout 之后', pos(eh))
os._exit(0)
