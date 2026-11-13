import ctypes, ctypes.wintypes as wt, os, sys
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, ppApp, pp_events as E
u = ctypes.WinDLL('user32', use_last_error=True)
u.GetClassNameW.argtypes=[wt.HWND, wt.LPWSTR, ctypes.c_int]
u.GetDlgCtrlID.argtypes=[wt.HWND]; u.GetDlgCtrlID.restype=ctypes.c_int
u.GetWindow.argtypes=[wt.HWND, wt.UINT]; u.GetWindow.restype=wt.HWND
u.GetParent.argtypes=[wt.HWND]; u.GetParent.restype=wt.HWND
u.SendMessageW.argtypes=[wt.HWND, wt.UINT, wt.WPARAM, ctypes.c_void_p]
u.SendMessageW.restype=ctypes.c_ssize_t
u.GetWindowRect.argtypes=[wt.HWND, ctypes.POINTER(wt.RECT)]

app = ppApp.App(); app.win.show(); M.pump(200); app.showEditor(); M.pump(400)
hits=[]
def walk(h):
    for c in iter_children(h):
        cid = u.GetDlgCtrlID(c)
        cls = ctypes.create_unicode_buffer(64); u.GetClassNameW(c, cls, 64)
        if cid == ppApp.TBL or cls.value=='SysListView32':
            r=wt.RECT(); u.GetWindowRect(c, ctypes.byref(r))
            hits.append((c, cid, cls.value, (r.left,r.top,r.right-r.left,r.bottom-r.top)))
        walk(c)
def iter_children(h):
    out=[]; c=u.GetWindow(h,5)
    while c: out.append(c); c=u.GetWindow(c,2)
    return out
walk(app.win.hwnd)
for h,cid,cls,rc in hits: print('TBL 候选 hwnd=%s id=%s cls=%s rect=%s' % (hex(h),cid,cls,rc))
print('cellRect API =', app.win.cellRect(ppApp.TBL, 5, 1))
# 直接测 GETSUBITEMRECT
LVM_GETSUBITEMRECT=0x1038
for h,cid,cls,rc in hits:
    if cls!='SysListView32': continue
    class R(ctypes.Structure):
        _fields=[('left',ctypes.c_long),('top',ctypes.c_long),('right',ctypes.c_long),('bottom',ctypes.c_long)]
    for row in (0,5,33):
        r=R(1,0,0,0)
        ok=u.SendMessageW(h, LVM_GETSUBITEMRECT, row, ctypes.byref(r))
        print('  GETSUBITEMRECT row=%d ok=%d rect=(%d,%d,%d,%d)' % (row, ok, r.left,r.top,r.right,r.bottom))
os._exit(0)
