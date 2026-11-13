import sys, os, time, ctypes
sys.path.insert(0, r'G:\PyArx\X64\Release'); os.add_dll_directory(r'G:\PyArx\X64\Release')
sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
os.environ['PYARX_D2D_RT'] = 'software'
from ctypes import wintypes as wt
M = __import__('PyMfcXaml')
u32 = ctypes.WinDLL('user32', use_last_error=True)
u32.FindWindowW.restype = wt.HWND
u32.GetDlgItem.restype = wt.HWND
u32.GetDlgItem.argtypes = [wt.HWND, ctypes.c_int]
u32.InvalidateRect.argtypes = [wt.HWND, ctypes.c_void_p, wt.BOOL]
u32.UpdateWindow.argtypes = [wt.HWND]
import pp_app
app = pp_app.PPApp(); app.win.show(); M.pump(80); app.showEditor(); M.pump(200)
hwnd = u32.FindWindowW(None, '配电规划图 — MFCXaml')
def rep(he, n=6):
    ts=[]
    for _ in range(n):
        t0=time.perf_counter(); u32.InvalidateRect(he,None,False); u32.UpdateWindow(he)
        ts.append((time.perf_counter()-t0)*1000.0)
    return ts
for cid,name in [(pp_app.ED_PANEL,'ED_PANEL白卡'),(pp_app.TBL,'TBL表格'),(pp_app.CV,'CV画布'),(pp_app.L_STATUS,'L_STATUS状态栏')]:
    he = u32.GetDlgItem(hwnd,int(cid))
    if he:
        ts=rep(he); print('%s 首帧=%.1f 后续=%s 均值=%.1f'%(name,ts[0],' '.join('%.1f'%t for t in ts[1:]),sum(ts[1:])/len(ts[1:])))
    else: print('%s 无HWND'%name)
ts=[]
for _ in range(5):
    t0=time.perf_counter(); u32.InvalidateRect(hwnd,None,False); u32.UpdateWindow(hwnd); ts.append((time.perf_counter()-t0)*1000.0)
print('父窗口整窗 %s'%' '.join('%.1f'%t for t in ts))
os._exit(0)
