import ctypes, os, sys
from ctypes import wintypes
import PyMfcXaml as p

HERE = os.path.dirname(os.path.abspath(__file__))
cmds = sys.argv[1] if len(sys.argv) > 1 else 'Clear #232A33'
xaml_path = os.path.join(HERE, '_one.xaml')
with open(xaml_path, 'w', encoding='utf-8') as f:
    f.write('<?xml version="1.0" encoding="utf-8"?>\n'
            '<Window xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Title="one" Width="240" Height="140">\n'
            '  <StackPanel Orientation="Vertical">\n'
            '    <QCanvas x:Name="qc" Width="220" Height="110" Commands="%s" />\n'
            '  </StackPanel>\n</Window>\n' % cmds)

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
w = p.guiload(xaml_path, names=names)
w.show(); p.pump(700)
out = []
EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
u.EnumChildWindows(w.hwnd, EP(lambda h, l: (out.append((h, u.GetDlgCtrlID(h))) or True)), 0)
cid = names['qc']
bi = BMIH(); bi.biSize = ctypes.sizeof(BMIH); bi.biWidth = 220; bi.biHeight = -110; bi.biPlanes = 1; bi.biBitCount = 32
bits = ctypes.c_void_p(); bmp = g.CreateDIBSection(0, ctypes.byref(bi), 0, ctypes.byref(bits), 0, 0)
mdc = g.CreateCompatibleDC(0); g.SelectObject(mdc, bmp)
u.SendMessageW([h for h, i in out if i == cid][0], 0x0318, mdc, 4)
print('cmds:', cmds)
pts = [(60, 45), (130, 45), (30, 30), (200, 60)]
if len(sys.argv) > 2:
    pts = []
    for a in sys.argv[2:]:
        x, y = a.split('x')
        pts.append((int(x), int(y)))
for pt in pts:
    v = g.GetPixel(mdc, pt[0], pt[1])
    print('  px', pt, (v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF))
for y in range(5, 110, 12):
    row = ''
    for x in range(5, 220, 12):
        v = g.GetPixel(mdc, x, y)
        c = (v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF)
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
w.close(); p.pump(200)
p.shutdown()
