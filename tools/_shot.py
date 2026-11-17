# 临时截图工具：抓 UIDesign 主窗位图存 PNG（Pillow + ctypes，避免 .NET 10 程序集拆分问题）
import ctypes, sys
from ctypes import wintypes
from PIL import Image

u32 = ctypes.windll.user32
gdi = ctypes.windll.gdi32

class RECT(ctypes.Structure):
    _fields_ = [("l", wintypes.LONG), ("t", wintypes.LONG),
               ("r", wintypes.LONG), ("b", wintypes.LONG)]

def shot(hwnd, out):
    rc = RECT(); u32.GetWindowRect(hwnd, ctypes.byref(rc))
    w, h = rc.r - rc.l, rc.b - rc.t
    hdc = u32.GetDC(hwnd)
    mem = gdi.CreateCompatibleDC(hdc)
    bmp = gdi.CreateCompatibleBitmap(hdc, w, h)
    gdi.SelectObject(mem, bmp)
    u32.PrintWindow(hwnd, mem, 3)
    class BI(ctypes.Structure):
        _fields_ = [("size", wintypes.DWORD), ("w", wintypes.LONG), ("h", wintypes.LONG),
                    ("planes", wintypes.WORD), ("bit", wintypes.WORD),
                    ("comp", wintypes.DWORD), ("img", wintypes.DWORD),
                    ("xpp", wintypes.LONG), ("ypp", wintypes.LONG),
                    ("clr", wintypes.DWORD), ("imp", wintypes.DWORD)]
    bi = BI(); bi.size = ctypes.sizeof(BI); bi.w = w; bi.h = -h
    bi.planes = 1; bi.bit = 32; bi.comp = 0
    buf = ctypes.create_string_buffer(w * h * 4)
    gdi.GetDIBits(mem, bmp, 0, h, buf, ctypes.byref(bi), 0)
    gdi.DeleteObject(bmp); gdi.DeleteDC(mem); u32.ReleaseDC(hwnd, hdc)
    Image.frombuffer("RGB", (w, h), buf, "raw", "BGRX").save(out)
    print("saved", out, w, h)

if __name__ == "__main__":
    name = sys.argv[1] if len(sys.argv) > 1 else "UIDesign"
    out = sys.argv[2] if len(sys.argv) > 2 else "docs/_shot.png"
    import subprocess
    ids = subprocess.check_output(["tasklist", "/FI", f"IMAGENAME eq {name}.exe", "/FO", "CSV", "/NH"]).decode()
    first = [x for x in ids.splitlines() if x.strip()]
    if not first:
        print("process not found"); sys.exit(1)
    pid = int(first[0].split('","')[1].strip('"'))
    EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    target = [None]
    def cb(h, l):
        p = wintypes.DWORD()
        u32.GetWindowThreadProcessId(h, ctypes.byref(p))
        if p.value == pid and u32.IsWindowVisible(h):
            target[0] = h; return False
        return True
    u32.EnumWindows(EnumWindowsProc(cb), 0)
    shot(target[0], out)
