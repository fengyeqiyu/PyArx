import ctypes, ctypes.wintypes as wt, sys, time
sys.path.insert(0, r'G:\PyArx\X64\Release'); sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import PyMfcXaml as M, pp_app
u = ctypes.WinDLL('user32', use_last_error=True)
u.PostMessageW.argtypes=[wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
CB=ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
kids=[]
def cb(h,l):
    buf=ctypes.create_unicode_buffer(64); u.GetClassNameW(h, buf, 64)
    r=wt.RECT(); u.GetWindowRect(h, ctypes.byref(r))
    kids.append((h, buf.value, r.right-r.left, r.bottom-r.top)); return True
app=pp_app.App(); app.win.show(); M.pump(60)
u.EnumChildWindows(app.win.hwnd, cb( cb ), 0)
print('子窗口:', [(k[1],k[2],k[3]) for k in kids], flush=True)
tbl=[k for k in kids if 'Table' in k[1]]
print('表格:', tbl, flush=True)
if tbl:
    h=tbl[0][0]; w,hh=tbl[0][2],tbl[0][3]
    for i,row in enumerate([0,1,5,20,38]):
        y = 24 + 21*row + 10
        u.PostMessageW(h, 0x0201, 1, (y<<16)|20)   # LBUTTONDOWN
        u.PostMessageW(h, 0x0202, 0, (y<<16)|20)   # LBUTTONUP
        M.pump(20)
        print('点击表格第%d行 ok'%row, flush=True)
    for d in (120,-120):
        u.PostMessageW(h, 0x020A, (120<<16) if d>0 else ((-120 & 0xFFFF)<<16), (30<<16)|50)
        M.pump(20); print('滚轮 %d ok'%d, flush=True)
print('全部完成，进程存活', flush=True)
