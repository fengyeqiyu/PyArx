# 第61轮：<div style="border-radius:N"> 必须有真圆角。
# HTML 里圆角是最基础的外观能力；此前 XML 的 <div> 画出来永远是直角块
# （背板由 WM_CTLCOLORSTATIC 纯色刷子出图，RoundRect 路径走不到）。
# 现在的实现：控件窗口按 CSS 半径设 window region，圆角外露出父窗口底色。
# 断言用 Win32 的 region，不依赖截图采样（像素断言在主题色上不稳）。
import os, sys, ctypes
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from ctypes import wintypes as wt

U = ctypes.windll.user32
G = ctypes.windll.gdi32
U.GetWindowRgn.argtypes = [wt.HWND, wt.HRGN]; U.GetWindowRgn.restype = ctypes.c_int
G.PtInRegion.argtypes = [wt.HRGN, ctypes.c_int, ctypes.c_int]; G.PtInRegion.restype = wt.BOOL
G.CreateRectRgn.argtypes = [ctypes.c_int] * 4; G.CreateRectRgn.restype = wt.HRGN
G.DeleteObject.argtypes = [wt.HRGN]
U.GetClientRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

def probe(radius, tag):
    path = "G:/PyArx/log/_dr_%s.xml" % tag
    xml = ('<XWindow Width="300" Height="200" Caption="r">'
           '<div Name="d" Left="20" Top="20" Width="200" Height="120" '
           'style="background:#335577;border-radius:%s"/></XWindow>') % radius
    with open(path, "w", encoding="utf-8") as f: f.write(xml)
    win = M.guiload(path); M.pump(300)
    found = []
    cb = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def visit(h, _):
        rc = wt.RECT(); U.GetClientRect(h, ctypes.byref(rc))
        if rc.right == 200 and rc.bottom == 120:
            rgn = G.CreateRectRgn(0, 0, 0, 0)
            t = U.GetWindowRgn(h, rgn)
            found.append((t, G.PtInRegion(rgn, 1, 1), G.PtInRegion(rgn, rc.right // 2, rc.bottom // 2)))
            G.DeleteObject(rgn)
        return True
    U.EnumChildWindows(win.hwnd, cb(visit), 0)
    win.close()
    return found[0] if found else None

r0 = probe("0", "a")
chk("border-radius:0 不裁剪（直角）", r0 is not None and r0[0] == 0, r0)
r12 = probe("12", "b")
chk("border-radius:12 设出圆角 region", r12 is not None and r12[0] > 0, r12)
chk("圆角外(1,1)在 region 之外", r12 is not None and r12[1] == 0, r12[1] if r12 else None)
chk("盒心仍在 region 之内", r12 is not None and r12[2] == 1, r12[2] if r12 else None)

print("t_div_radius.py: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
