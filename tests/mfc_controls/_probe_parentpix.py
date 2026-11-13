import ctypes
from ctypes import wintypes
import PyMfcXaml as p

u = ctypes.windll.user32
g = ctypes.windll.gdi32


def proto(fn, args, res):
    for lib, name in fn:
        f = getattr(lib, name)
        f.argtypes = args
        f.restype = res


u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
u.GetDC.argtypes = [ctypes.c_void_p]
u.GetDC.restype = wintypes.HDC
u.ReleaseDC.argtypes = [ctypes.c_void_p, wintypes.HDC]
u.GetClientRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.GetParent.argtypes = [ctypes.c_void_p]
u.GetParent.restype = ctypes.c_void_p
u.MapWindowPoints.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
g.CreateCompatibleDC.argtypes = [wintypes.HDC]
g.CreateCompatibleDC.restype = wintypes.HDC
g.CreateCompatibleBitmap.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int]
g.CreateCompatibleBitmap.restype = wintypes.HBITMAP
g.SelectObject.argtypes = [wintypes.HDC, wintypes.HGDIOBJ]
g.SelectObject.restype = wintypes.HGDIOBJ
g.BitBlt.argtypes = [wintypes.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
                     wintypes.HDC, ctypes.c_int, ctypes.c_int, wintypes.DWORD]
g.DeleteObject.argtypes = [wintypes.HGDIOBJ]
g.DeleteDC.argtypes = [wintypes.HDC]
g.GetDIBits.argtypes = [wintypes.HDC, wintypes.HBITMAP, wintypes.UINT, wintypes.UINT,
                        ctypes.c_void_p, ctypes.c_void_p, wintypes.UINT]


class BIH(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                ('biPlanes', wintypes.WORD), ('biBitCount', wintypes.WORD),
                ('biCompression', wintypes.DWORD), ('biSizeImage', wintypes.DWORD),
                ('a', ctypes.c_long), ('b', ctypes.c_long), ('c', wintypes.DWORD), ('d', wintypes.DWORD)]


class BI(ctypes.Structure):
    _fields_ = [('h', BIH), ('col', wintypes.DWORD * 3)]


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long), ('r', ctypes.c_long), ('b', ctypes.c_long)]


EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def byid(top, cid):
    out = []

    def cb(h, _):
        if u.GetDlgCtrlID(h) == cid:
            out.append(h)
        return True
    u.EnumChildWindows(top, EP(cb), 0)
    return out[0] if out else 0


def grab(hwnd, x, y, wd, ht):
    if wd <= 0 or ht <= 0:
        return b''
    dc = u.GetDC(hwnd)
    m = g.CreateCompatibleDC(dc)
    bmp = g.CreateCompatibleBitmap(dc, wd, ht)
    o = g.SelectObject(m, bmp)
    g.BitBlt(m, 0, 0, wd, ht, dc, x, y, 0x00CC0020)
    buf = ctypes.create_string_buffer(wd * ht * 4)
    bi = BI()
    bi.h.biSize = ctypes.sizeof(BIH)
    bi.h.biWidth = wd
    bi.h.biHeight = -ht
    bi.h.biPlanes = 1
    bi.h.biBitCount = 32
    g.GetDIBits(m, bmp, 0, ht, buf, ctypes.byref(bi), 0)
    g.SelectObject(m, o)
    g.DeleteObject(bmp)
    g.DeleteDC(m)
    u.ReleaseDC(hwnd, dc)
    return buf.raw


def grab_child(parent, child):
    """控件是 owner-draw：真正落笔在父窗口的 WM_DRAWITEM 里，所以要从父窗口取像素。"""
    wr = R()
    u.GetWindowRect(child, ctypes.byref(wr))
    pts = (R(wr.l, wr.t, wr.r, wr.b))
    px, py = ctypes.c_long(wr.l), ctypes.c_long(wr.t)
    u.MapWindowPoints(0, parent, ctypes.byref(pts), 2)
    return grab(parent, pts.l, pts.t, pts.r - pts.l, pts.b - pts.t)


def ratio(a, b):
    if not a or not b:
        return -1.0
    n = min(len(a), len(b))
    c = sum(1 for i in range(0, n, 4) if a[i:i + 3] != b[i:i + 3])
    return c / (n // 4) * 100.0


names = {}
w = p.guiload(r'G:\PyArx\tests\mfc_controls\_fix1.xaml', names=names)
w.show()
p.pump(500)

for key, label, lo, hi, msg, expect in (('chkFlag', 'CheckBox', 0, 1, 0x00F0, 1),
                                        ('prgLoad', 'ProgressBar', 5, 92, 0x0408, 92),
                                        ('sldZoom', 'Slider', 5, 95, 0x0400, 95)):
    h = byid(w.hwnd, names[key])
    parent = u.GetParent(h)
    if key == 'chkFlag':
        w.setCheck(names[key], False)
    else:
        w.setPos(names[key], lo)
    p.pump(300)
    a = grab_child(parent, h)
    if key == 'chkFlag':
        w.setCheck(names[key], True)
    else:
        w.setPos(names[key], hi)
    p.pump(400)
    b = grab_child(parent, h)
    state = u.SendMessageW(h, msg, 0, 0)
    print('%s 状态=%d 父窗口区域像素变化=%.2f%%（非零像素 a=%d b=%d）' % (
        label, state, ratio(a, b), sum(1 for i in range(0, len(a), 4) if a[i:i + 3] != b'\x00\x00\x00'),
        sum(1 for i in range(0, len(b), 4) if b[i:i + 3] != b'\x00\x00\x00')), flush=True)

w.close()
p.pump(150)
p.shutdown()
