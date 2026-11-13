import sys, os, time, ctypes
from ctypes import wintypes as wt
sys.path.insert(0, r'G:\PyArx\X64\Release'); sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import pp_app
u = ctypes.windll.user32
P = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
app = pp_app.App(); app.win.show(); app.showEditor()
t=time.perf_counter(); app.onSampleDoc(); print('sample %.0f ms' % ((time.perf_counter()-t)*1000), flush=True)
time.sleep(0.3)
u.ShowWindow(app.win.hwnd, 9)
u.SetWindowPos(app.win.hwnd, ctypes.c_void_p(-1), 150, 150, 1440, 1168, 0x0050)
u.SetForegroundWindow(app.win.hwnd); time.sleep(1.0)
def children(root):
    out=[]
    def cb(h,l):
        out.append(h); return True
    u.EnumChildWindows(root, P(cb), None); return out
tbl=[h for h in children(app.win.hwnd) if u.GetDlgCtrlID(h)==3021][0]
def click(x,y):
    lp=(y<<16)|(x&0xFFFF)
    u.SendMessageW(tbl,0x0201,1,lp); u.SendMessageW(tbl,0x0202,0,lp)
click(350,15)          # 「型号」列漏斗
u.UpdateWindow(tbl)
print('panel opened, view', app.win.tableViewRows(3021), flush=True)
time.sleep(5.0)
print('rows after', app.win.tableRowCount(3021), 'view', app.win.tableViewRows(3021), flush=True)
time.sleep(2.0)
