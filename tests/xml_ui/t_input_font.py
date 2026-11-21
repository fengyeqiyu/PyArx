# 第62轮：输入框（原生 EDIT）的字形与双击选词
# 缺口：XML 写了 font-size / font-family，自绘件（label/button）按外观表画字，
# 但输入框是真 EDIT —— 它用 WM_SETFONT 那把 HFONT 画。此前 XML 字号从未下发，
# 输入框恒为宿主默认 10pt，点进点出/失焦聚焦就可能换字形；顺带验证双击选词没被吞。
import os, sys, ctypes
from ctypes import wintypes
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M

XML = '''<window xmlns:x="http://schemas.microsoft.com/winfx/2006/xml" Width="360" Height="190" Background="#313842">
  <div style="left:20;top:16;width:300;height:64;background:#263238">
    <input x:Name="e2" value="Alpha Beta" style="left:12;top:16;width:250;height:30;font-size:15;font-family:'Microsoft YaHei'"/>
  </div>
  <input x:Name="e1" value="Hello World" style="left:20;top:110;width:250;height:30;font-size:16;font-family:'Microsoft YaHei'"/>
</window>'''

u = ctypes.windll.user32
gdi = ctypes.windll.gdi32
u.GetWindow.argtypes = [wintypes.HWND, ctypes.c_uint]
u.GetWindow.restype = wintypes.HWND
u.SendMessageW.argtypes = [wintypes.HWND, ctypes.c_uint, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.SetFocus.argtypes = [wintypes.HWND]
u.SetFocus.restype = wintypes.HWND
u.GetDlgCtrlID.argtypes = [wintypes.HWND]
u.GetDlgCtrlID.restype = ctypes.c_int

WM_GETFONT, WM_SETFOCUS, EM_GETSEL = 0x0031, 0x0007, 0x00B0
WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK = 0x0201, 0x0202, 0x0203


class LOGFONTW(ctypes.Structure):
    _fields_ = [('lfHeight', ctypes.c_long), ('lfWidth', ctypes.c_long),
                ('lfEscapement', ctypes.c_long), ('lfOrientation', ctypes.c_long),
                ('lfWeight', ctypes.c_long), ('lfItalic', ctypes.c_byte),
                ('lfUnderline', ctypes.c_byte), ('lfStrikeOut', ctypes.c_byte),
                ('lfCharSet', ctypes.c_byte), ('lfOutPrecision', ctypes.c_byte),
                ('lfClipPrecision', ctypes.c_byte), ('lfQuality', ctypes.c_byte),
                ('lfPitchAndFamily', ctypes.c_byte), ('lfFaceName', ctypes.c_wchar * 32)]


gdi.GetObjectW.argtypes = [wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p]
gdi.GetObjectW.restype = ctypes.c_int


def font_of(h):
    hf = u.SendMessageW(h, WM_GETFONT, 0, 0)
    if not hf:
        return None
    lf = LOGFONTW()
    gdi.GetObjectW(hf, ctypes.sizeof(lf), ctypes.byref(lf))
    return (lf.lfFaceName, -lf.lfHeight, lf.lfWeight)


def all_desc(h):
    out, c = [], u.GetWindow(h, 5)
    while c:
        out.append(c)
        out += all_desc(c)
        c = u.GetWindow(c, 2)
    return out


def click(h, x, y, dbl=False):
    lp = (y << 16) | (x & 0xFFFF)
    u.SendMessageW(h, WM_LBUTTONDOWN, 1, lp)
    u.SendMessageW(h, WM_LBUTTONUP, 0, lp)
    if dbl:
        u.SendMessageW(h, WM_LBUTTONDBLCLK, 1, lp)
        u.SendMessageW(h, WM_LBUTTONUP, 0, lp)


p = r'G:\PyArx\log\_t_input_font.xml'
open(p, 'w', encoding='utf-8').write(XML)
names = {}
w = M.guiload(p, context={}, names=names)
w.show()
M.pump(400)
byid = {}
for c in all_desc(wintypes.HWND(int(w.hwnd))):
    byid[u.GetDlgCtrlID(c)] = c
e1, e2 = byid.get(names.get('e1')), byid.get(names.get('e2'))
OK = True


def check(cond, msg):
    global OK
    OK &= bool(cond)
    print(('PASS ' if cond else 'FAIL ') + msg)


check(e1 is not None and e2 is not None, '两个输入框都拿到句柄 e1=%s e2=%s' % (e1, e2))

f0 = font_of(e1)
check(f0 is not None and 'YaHei' in f0[0] and f0[1] == 16,
      'XML 的 font-family/font-size 落到原生输入框（实际 %s）' % (f0,))

u.SetFocus(e1)
u.SendMessageW(e1, WM_SETFOCUS, 0, 0)
M.pump(120)
f1 = font_of(e1)
check(f1 == f0, '聚焦后字体不变（前 %s / 后 %s）' % (f0, f1))

click(e1, 48, 14)
M.pump(120)
f2 = font_of(e1)
check(f2 == f0, '点击进去后字体不变（前 %s / 后 %s）' % (f0, f2))

r = u.SendMessageW(e1, EM_GETSEL, 0, 0)
s, t = r & 0xFFFF, (r >> 16) & 0xFFFF
check(s == t, '单击只落光标、不产生选区（start=%d end=%d）' % (s, t))

click(e1, 48, 14, dbl=True)
M.pump(120)
r = u.SendMessageW(e1, EM_GETSEL, 0, 0)
s, t = r & 0xFFFF, (r >> 16) & 0xFFFF
check(t > s and s == 0 and t <= 6, '双击选中了第一个词 Hello（start=%d end=%d）' % (s, t))

f3 = font_of(e2)
check(f3 is not None and 'YaHei' in f3[0] and f3[1] == 15,
      '容器内输入框同样按 XML 字号下发（实际 %s）' % (f3,))

click(e2, 40, 14, dbl=True)
M.pump(120)
r = u.SendMessageW(e2, EM_GETSEL, 0, 0)
s2, t2 = r & 0xFFFF, (r >> 16) & 0xFFFF
check(t2 > s2 and s2 == 0 and t2 <= 6, '卡片内输入框双击也能选词 Alpha（start=%d end=%d）' % (s2, t2))

check(not names.get('e1') is None, 'names 里有 x:Name 映射')
w.save_png(r'G:\PyArx\log\_t_input_font.png')
w.close()
print('t_input_font: %s' % ('PASS=8 FAIL=0' if OK else 'FAIL'))
