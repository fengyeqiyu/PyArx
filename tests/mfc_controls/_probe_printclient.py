import ctypes, sys, time
from ctypes import wintypes
import PyMfcXaml as p

u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
g.CreateCompatibleDC.argtypes = [wintypes.HDC]
g.CreateCompatibleDC.restype = wintypes.HDC
g.CreateCompatibleBitmap.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int]
g.CreateCompatibleBitmap.restype = wintypes.HBITMAP
g.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
g.SelectObject.restype = wintypes.HGDIOBJ
g.DeleteObject.argtypes = [wintypes.HGDIOBJ]
g.DeleteDC.argtypes = [wintypes.HDC]
g.GetDIBits.argtypes = [wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT,
                        ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', ctypes.c_int), ('biHeight', ctypes.c_int),
                ('biPlanes', wintypes.WORD), ('biBitCount', wintypes.WORD),
                ('biCompression', wintypes.DWORD), ('biSizeImage', wintypes.DWORD),
                ('biXPelsPerMeter', ctypes.c_int), ('biYPelsPerMeter', ctypes.c_int),
                ('biClrUsed', wintypes.DWORD), ('biClrImportant', wintypes.DWORD)]


class BITMAPINFO(ctypes.Structure):
    _fields_ = [('bmiHeader', BITMAPINFOHEADER), ('bmiColors', wintypes.DWORD * 3)]


class RECT(ctypes.Structure):
    _fields_ = [('left', ctypes.c_long), ('top', ctypes.c_long),
                ('right', ctypes.c_long), ('bottom', ctypes.c_long)]


def by_id(hwnd, cid):
    got = []
    EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(h, _):
        if u.GetDlgCtrlID(h) == cid:
            got.append(h)
        return True
    u.EnumChildWindows(hwnd, EP(cb), 0)
    return got[0] if got else None


def print_client(hwnd):
    r = RECT()
    u.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    hdc = u.GetDC(0)
    mdc = g.CreateCompatibleDC(hdc)
    bmp = g.CreateCompatibleBitmap(hdc, w, h)
    g.SelectObject(mdc, bmp)
    u.SendMessageW(hwnd, 0x0318, mdc, 0x0004)  # WM_PRINTCLIENT + PRF_CLIENT
    buf = ctypes.create_string_buffer(w * h * 4)
    bi = BITMAPINFO()
    bi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bi.bmiHeader.biWidth = w
    bi.bmiHeader.biHeight = -h
    bi.bmiHeader.biPlanes = 1
    bi.bmiHeader.biBitCount = 32
    g.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    g.DeleteObject(bmp)
    g.DeleteDC(mdc)
    u.ReleaseDC(0, hdc)
    return buf.raw


def ratio(a, b):
    n = min(len(a), len(b))
    ch = sum(1 for i in range(0, n, 4) if a[i:i + 3] != b[i:i + 3])
    return ch / max(1, n // 4) * 100.0


names = {}
w = p.guiload(r'G:\PyArx\tests\mfc_controls\_fix1.xaml', names=names)
w.show()
p.pump(400)

for key, lo, hi, posmsg in (('prgLoad', 5, 92, 0x0408), ('sldZoom', 5, 95, 0x0400),
                            ('chkFlag', 0, 1, 0x00F0)):
    h = by_id(w.hwnd, names[key])
    if key == 'chkFlag':
        w.setCheck(names[key], False)
    else:
        w.setPos(names[key], lo)
    p.pump(250)
    a = print_client(h)
    if key == 'chkFlag':
        w.setCheck(names[key], True)
    else:
        w.setPos(names[key], hi)
    p.pump(250)
    b = print_client(h)
    print('%s 内部绘制变化 %.2f%%  状态=%d' % (key, ratio(a, b), u.SendMessageW(h, posmsg, 0, 0)))

w.close()
p.pump(150)
p.shutdown()
sys.exit(0)
