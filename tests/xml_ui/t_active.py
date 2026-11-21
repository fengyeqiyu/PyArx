# 第55轮：CSS :active 的 XML 等价（active-background / active-color）
# HTML `button:active{background:#...}` 按下就变；XML 早先只有宿主主题的固定按下色，改不了。
import os, sys, ctypes
BIN = os.environ["PYARX_BIN"]; sys.path.insert(0, BIN); os.add_dll_directory(BIN)
os.environ.setdefault("PYARX_D2D_RT", "software")
import PyMfcXml as M
from ctypes import wintypes as wt
from PIL import Image

fails = []
def chk(name, cond, got=""):
    print(("PASS  " if cond else "FAIL  ") + name + ("  " + str(got) if got else ""))
    if not cond: fails.append(name)

user32 = ctypes.WinDLL("user32", use_last_error=True)
WNDENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
user32.EnumChildWindows.argtypes = [wt.HWND, WNDENUMPROC, wt.LPARAM]
user32.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = wt.LPARAM
user32.RedrawWindow.argtypes = [wt.HWND, ctypes.c_void_p, ctypes.c_void_p, wt.UINT]

def red_px(p):
    im = Image.open(p).convert("RGB")
    return sum(1 for y in range(34, im.size[1]) for x in range(10, im.size[0])
               if im.getpixel((x, y))[0] > 150 and im.getpixel((x, y))[1] < 90
               and im.getpixel((x, y))[2] < 90)

def find(hwnd, text):
    got = []
    def cb(h, lp):
        buf = ctypes.create_unicode_buffer(64)
        user32.GetWindowTextW(h, buf, 64)
        if buf.value.strip().upper() == text:
            got.append(h)
        return True
    user32.EnumChildWindows(int(hwnd), WNDENUMPROC(cb), 0)
    return got

def shot(tag, press):
    n = {}
    w = M.guiload_text('<XWindow Width="260" Height="90">'
                       '<button Name="b" Left="20" Top="20" Width="120" Height="36" Text="PRESS" '
                       'style="active-color:#ff0000"/></XWindow>', context=globals(), names=n)
    M.pump(200)
    if press:
        f = find(w.hwnd, "PRESS")
        if f:
            user32.SendMessageW(f[0], 0x00F3, 1, 0)  # BM_SETSTATE TRUE
            user32.RedrawWindow(int(w.hwnd), None, None, 0x0085)
            M.pump(200)
    p = "G:/PyArx/log/%s.png" % tag
    w.save_png(p)
    v = w.get_xml_value(n["b"], "ActiveColor")
    w.close()
    return v, red_px(p)

v0, r0 = shot("ac_off", False)
v1, r1 = shot("ac_on", True)
chk("ActiveColor 契约回读", v0 == "#ff0000" and v1 == "#ff0000", (v0, v1))
chk("按下后文字变红（常态无红像素）", r0 == 0 and r1 > 100, (r0, r1))
print("t_active.py: PASS=%d FAIL=%d" % (2 - len(fails), len(fails)))
