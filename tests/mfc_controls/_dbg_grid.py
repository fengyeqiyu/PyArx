import ctypes, os
from ctypes import wintypes
import PyMfcXaml as p

HERE = os.path.dirname(os.path.abspath(__file__))
u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]; u.GetDlgCtrlID.restype = ctypes.c_int
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
g.GetPixel.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]; g.GetPixel.restype = wintypes.DWORD
g.CreateDIBSection.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD, ctypes.c_void_p, ctypes.c_void_p, wintypes.DWORD]
g.CreateDIBSection.restype = ctypes.c_void_p
g.SelectObject.argtypes = [ctypes.c_void_p, ctypes.c_void_p]; g.SelectObject.restype = ctypes.c_void_p
g.CreateCompatibleDC.argtypes = [ctypes.c_void_p]; g.CreateCompatibleDC.restype = ctypes.c_void_p

class BMIH(ctypes.Structure):
    _fields_ = [('biSize', wintypes.DWORD), ('biWidth', ctypes.c_long), ('biHeight', ctypes.c_long),
                ('biPlanes', wintypes.WORD), ('biBitCount', wintypes.WORD),
                ('biCompression', wintypes.DWORD), ('biSizeImage', wintypes.DWORD),
                ('biXPelsPerMeter', ctypes.c_long), ('biYPelsPerMeter', ctypes.c_long),
                ('biClrUsed', wintypes.DWORD), ('biClrImportant', wintypes.DWORD)]

names = {}
w = p.guiload(os.path.join(HERE, '_cmdmatrix.xaml'), names=names)
w.show(); p.pump(900)
out = []
EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumChildWindows(w.hwnd, EP(lambda h, l: (out.append((h, u.GetDlgCtrlID(h))) or True)), 0)

def grab(cid, ww, hh):
    kid = [h for h, i in out if i == cid][0]
    bi = BMIH(); bi.biSize = ctypes.sizeof(BMIH); bi.biWidth = ww; bi.biHeight = -hh; bi.biPlanes = 1; bi.biBitCount = 32
    bits = ctypes.c_void_p(); bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
    mdc = g.CreateCompatibleDC(0); g.SelectObject(mdc, bmp)
    u.SendMessageW(kid, 0x0318, mdc, 4)
    return lambda x, y: (lambda v: (v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF))(g.GetPixel(mdc, x, y))

for key in ('c5', 'c6', 'c7'):
    px = grab(names[key], 220, 110)
    print('=== %s ===' % key)
    for y in range(5, 110, 12):
        row = ''
        for x in range(5, 220, 12):
            c = px(x, y)
            if c == (35, 42, 51):
                row += '.'
            elif c[2] > 150 and c[0] < 60:
                row += 'B'
            elif c[0] > 200:
                row += 'R'
            elif sum(c) < 30:
                row += '#'
            else:
                row += 'x'
        print('%3d %s' % (y, row))
w.close(); p.pump(200); p.shutdown()
