import ctypes, ctypes.wintypes as wt, os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp, pp_events as E
u = ctypes.WinDLL('user32', use_last_error=True)
u.GetClassNameW.argtypes=[wt.HWND, wt.LPWSTR, ctypes.c_int]
u.GetParent.argtypes=[wt.HWND]; u.GetParent.restype=wt.HWND
u.GetWindowRect.argtypes=[wt.HWND, ctypes.POINTER(wt.RECT)]
u.GetClientRect.argtypes=[wt.HWND, ctypes.POINTER(wt.RECT)]
u.GetDlgCtrlID.argtypes=[wt.HWND]; u.GetDlgCtrlID.restype=ctypes.c_int
u.GetFocus.restype=wt.HWND

def info(h, tag):
    r=wt.RECT(); u.GetWindowRect(h, ctypes.byref(r))
    cr=wt.RECT(); u.GetClientRect(h, ctypes.byref(cr))
    cls=ctypes.create_unicode_buffer(64); u.GetClassNameW(h, cls, 64)
    p=u.GetParent(h); pr=wt.RECT()
    if p: u.GetWindowRect(p, ctypes.byref(pr)); pc=ctypes.create_unicode_buffer(64); u.GetClassNameW(p, pc, 64)
    else: pc=ctypes.create_unicode_buffer(8)
    print('%s id=%d cls=%s window=(%d,%d %dx%d) client=%dx%d parentid=%s parentcls=%s pwin=(%d,%d %dx%d)' % (
        tag, u.GetDlgCtrlID(h), cls.value, r.left,r.top,r.right-r.left,r.bottom-r.top,
        cr.right,cr.bottom, (u.GetDlgCtrlID(p) if p else '-'), pc.value or '-', pr.left,pr.top,pr.right-pr.left,pr.bottom-pr.top))

app = ppApp.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
info(app.win.hwnd, '主窗')
tbl = E.find_control(app.win.hwnd, ppApp.TBL); info(tbl, 'TBL')
rc = app.win.cellRect(ppApp.TBL, 5, 1); print('cellRect', rc, 'tbl_pos', app._tblPos)
E.double_click(tbl, rc[0]+rc[2]//2, rc[1]+rc[3]//2); M.pump(200)
ce = app._ce; print('ce', ce)
eh = E.find_control(app.win.hwnd, ce[2]); info(eh, 'EDIT')
print('focus id =', u.GetDlgCtrlID(u.GetFocus() or 0), ' edit id =', ce[2])
os._exit(0)
