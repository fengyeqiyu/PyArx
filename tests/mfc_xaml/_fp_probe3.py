import sys, time, ctypes
from ctypes import wintypes as wt
sys.path.insert(0, r'G:\PyArx\X64\Release'); sys.path.insert(0, r'G:\PyArx\tests\mfc_xaml')
import pp_app
u = ctypes.windll.user32
P = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
app = pp_app.App(); app.win.show(); app.showEditor(); app.onSampleDoc()
time.sleep(0.3)
u.ShowWindow(app.win.hwnd, 9)
u.SetWindowPos(app.win.hwnd, ctypes.c_void_p(-1), 150, 150, 1440, 1168, 0x0050)
time.sleep(0.6)
def children(root):
    out=[]
    def cb(h,l):
        out.append(h); return True
    u.EnumChildWindows(root, P(cb), None); return out
tbl=[h for h in children(app.win.hwnd) if u.GetDlgCtrlID(h)==3021][0]
def click(x,y):
    lp=(y<<16)|(x&0xFFFF)
    u.SendMessageW(tbl,0x0201,1,lp); u.SendMessageW(tbl,0x0202,0,lp)
print('rows', app.win.tableRowCount(3021), 'view', app.win.tableViewRows(3021), flush=True)
click(350,15)                  # 型号列漏斗
for y in (98,120):             # 取消「-」和「BV-3×6」
    click(360,y); time.sleep(0.1)
click(553,187)                 # 确定
time.sleep(0.3); u.UpdateWindow(tbl)
print('after filter view', app.win.tableViewRows(3021), flush=True)
