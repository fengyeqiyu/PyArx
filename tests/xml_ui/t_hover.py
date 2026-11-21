# 第44轮：CSS :hover（XML 等价写法 hover-background / hover-color）
# HTML 里 `button:hover{background:#E5484D}` 一行就有的交互反馈；XML 早先完全没有。
import os, sys, ctypes
from ctypes import wintypes as W
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

u = ctypes.WinDLL("user32", use_last_error=True)
u.EnumChildWindows.argtypes = [W.HWND, ctypes.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM), W.LPARAM]
u.GetWindowTextW.argtypes = [W.HWND, ctypes.c_wchar_p, ctypes.c_int]
u.SendMessageW.argtypes = [W.HWND, W.UINT, W.WPARAM, W.LPARAM]
u.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
u.GetWindowRect.argtypes = [W.HWND, ctypes.POINTER(W.RECT)]
u.SetForegroundWindow.argtypes = [W.HWND]
u.ShowWindow.argtypes = [W.HWND, ctypes.c_int]

XML = ('<XWindow Width="300" Height="90">'
       '<button Name="hb" Left="10" Top="20" Width="160" Height="36" Text="Hover" '
       'style="background:#2F353B;color:#FFFFFF;hover-background:#E5484D;hover-color:#E5484D">'
       '</button></XWindow>')

n = {}
win = M.guiload_text(XML, context=globals(), names=n)
M.pump(300)
hb = n["hb"]

chk("hover-background 回读", win.get_xml_value(hb, "HoverBackground") == "#E5484D",
    win.get_xml_value(hb, "HoverBackground"))
chk("hover-color 回读", win.get_xml_value(hb, "HoverColor") == "#E5484D",
    win.get_xml_value(hb, "HoverColor"))

target = []
@ctypes.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
def cb(h, lp):
    buf = ctypes.create_unicode_buffer(64)
    u.GetWindowTextW(h, buf, 64)
    if buf.value == "Hover":
        target.append(h)
    return True
u.EnumChildWindows(W.HWND(win.hwnd), cb, 0)
chk("找到按钮真实句柄", len(target) == 1, len(target))

def red_pixels(png):
    im = Image.open(png).convert("RGB")
    return sum(1 for px in im.getdata() if px[0] > 180 and px[1] < 110 and px[2] < 130)

u.SetCursorPos(40, 40)   # 光标先挪开，确保初始不是 hover 态
u.ShowWindow(W.HWND(win.hwnd), 5)   # SW_SHOW：不可见的窗口收不到鼠标移动
M.pump(300)
win.save_png("G:/PyArx/log/hv_off.png")
chk("常态没有 hover 红字", red_pixels("G:/PyArx/log/hv_off.png") == 0,
    red_pixels("G:/PyArx/log/hv_off.png"))

# 投递一条 WM_MOUSEMOVE 直接推进命中态（离开追踪只在 WM_SETCURSOR 登记，
# 所以投递消息不会被系统补发的 MOUSELEAVE 抵消）；随后立刻抓帧。
if target:
    u.SendMessageW(W.HWND(target[0]), 0x0200, 0, 0)  # WM_MOUSEMOVE
win.save_png("G:/PyArx/log/hv_on.png")
chk("移入后文字转 hover 色", red_pixels("G:/PyArx/log/hv_on.png") > 0,
    red_pixels("G:/PyArx/log/hv_on.png"))

# 离开：投递 WM_MOUSELEAVE（真实环境下由 WM_SETCURSOR 登记的 TME_LEAVE 自动送来）
if target:
    u.SendMessageW(W.HWND(target[0]), 0x02A3, 0, 0)
M.pump(200)
win.save_png("G:/PyArx/log/hv_back.png")
chk("移出后恢复常态", red_pixels("G:/PyArx/log/hv_back.png") == 0,
    red_pixels("G:/PyArx/log/hv_back.png"))

win.close()
print("t_hover: PASS=%d FAIL=%d" % (6 - len(fails), len(fails)))

