# 第54轮：CSS :focus 的 XML 等价（focus-background / focus-color）
# HTML 里 `input:focus{background:#...;color:#f00}` 一行到位；XML 此前只有宿主画的焦点环，
# 改不了获焦时的底色与字色。
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
user32.EnumChildWindows.restype = wt.BOOL
user32.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
user32.GetWindowTextW.restype = ctypes.c_int
user32.SendMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
user32.SendMessageW.restype = wt.LPARAM
user32.SetForegroundWindow.argtypes = [wt.HWND]
user32.SetFocus.argtypes = [wt.HWND]
user32.RedrawWindow.argtypes = [wt.HWND, ctypes.c_void_p, ctypes.c_void_p, wt.UINT]

def red_px(p):
    im = Image.open(p).convert("RGB")
    n = 0
    for y in range(34, im.size[1]):
        for x in range(10, im.size[0]):
            c = im.getpixel((x, y))
            if c[0] > 150 and c[1] < 90 and c[2] < 90:
                n += 1
    return n

def shot(tag, focus_it):
    n = {}
    w = M.guiload_text('<XWindow Width="260" Height="80">'
                       '<input Name="i" Left="20" Top="20" Width="200" Height="30" Text="FOCUS" '
                       'style="focus-color:#ff0000"/></XWindow>', context=globals(), names=n)
    M.pump(200)
    if focus_it:
        found = []
        def cb(h, lp):
            buf = ctypes.create_unicode_buffer(64)
            user32.GetWindowTextW(h, buf, 64)
            if buf.value.strip().upper() == "FOCUS":
                found.append(h)
            return True
        user32.EnumChildWindows(int(w.hwnd), WNDENUMPROC(cb), 0)
        if found:
            # 真实焦点：先抢前台再 SetFocus（跨窗口 SetFocus 需要前台权限），
            # 然后触发一次整窗重绘，让 Edit 的 Palette 按 focus 态重建。
            user32.SetForegroundWindow(int(w.hwnd))
            user32.SetFocus(found[0])
            user32.RedrawWindow(int(w.hwnd), None, None, 0x0085)  # INVALIDATE|ALLCHILDREN|UPDATENOW
            M.pump(200)
    p = "G:/PyArx/log/%s.png" % tag
    w.save_png(p)
    v = w.get_xml_value(n["i"], "FocusColor")
    w.close()
    return v, red_px(p)

v0, r0 = shot("fc_off", False)
v1, r1 = shot("fc_on", True)
chk("FocusColor 契约回读", v0 == "#ff0000" and v1 == "#ff0000", (v0, v1))
chk("获焦后文字变红（未获焦无红像素）", r0 == 0 and r1 > 100, (r0, r1))
print("t_focus.py: PASS=%d FAIL=%d" % (2 - len(fails), len(fails)))
