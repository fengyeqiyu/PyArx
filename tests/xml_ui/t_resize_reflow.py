# 第62轮：宿主尺寸变了要自己重排，不需要用户 onResize 重建界面。
# HTML 里 resize 是免费的；XML 通道此前得在 win.on_resize 里重新生成整棵树
# （cad2d 的 <XScene> 就是这么干的），书写成本和运行开销都多一档。
import os, sys, ctypes
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from ctypes import wintypes as wt

U = ctypes.windll.user32
U.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
U.GetWindowRect.argtypes = [wt.HWND, ctypes.POINTER(wt.RECT)]
U.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

XML = ('<XWindow Width="420" Height="300" Caption="rs">'
       '<div Name="row" Left="0" Top="0" Width="100%" Height="40" '
       'style="display:flex;width:100%;height:40">'
       '<label Name="a" style="flex:1" Text="ALPHA"/>'
       '<label Name="b" style="flex:1" Text="BETA"/>'
       '</div></XWindow>')
path = "G:/PyArx/log/_rs.xml"
with open(path, "w", encoding="utf-8") as f: f.write(XML)

win = M.guiload(path); M.pump(400)

def find_by_text(txt):
    hits = []
    cb = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def visit(h, _):
        buf = ctypes.create_unicode_buffer(64)
        U.GetWindowTextW(h, buf, 64)
        if buf.value.strip() == txt:
            rc = wt.RECT(); U.GetWindowRect(h, ctypes.byref(rc))
            hits.append((h, rc.left, rc.right, rc.bottom - rc.top))
        return True
    U.EnumChildWindows(win.hwnd, cb(visit), 0)
    return hits[0] if hits else None

a0 = find_by_text("ALPHA")
chk("初始找到子控件", a0 is not None, a0)

SWP_NOMOVE, SWP_NOZORDER = 0x0002, 0x0004
U.SetWindowPos(win.hwnd, None, 0, 0, 640, 300, SWP_NOMOVE | SWP_NOZORDER)
M.pump(500)

a1 = find_by_text("ALPHA")
chk("窗口变宽后子控件仍在", a1 is not None, a1)
if a0 and a1:
    w0, w1 = a0[2] - a0[1], a1[2] - a1[1]
    chk("子控件宽度随宿主重排", w1 > w0 + 20, "before=%d after=%d" % (w0, w1))
    b1 = find_by_text("BETA")
    if b1 is not None:
        chk("flex:1 两半仍等宽（差<6px）", abs(w1 - (b1[2] - b1[1])) < 6,
            "a=%d b=%d" % (w1, b1[2] - b1[1]))
win.close()
print("t_resize_reflow.py: PASS=%d FAIL=%d" % (4 - len(fails), len(fails)))
