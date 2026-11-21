# 第59轮：CSS position:sticky（元素跟着滚，滚到吸顶线就钉住）
# 缺口：XML 侧此前只有 fixed（一滚到底不动），"滚动时吸顶的标题栏/表头"这种 HTML 常规写法表达不了。
import os, sys, ctypes
from ctypes import wintypes
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M

XML = '''<window Width="420" Height="320" Background="#313842">
  <div style="left:20;top:20;width:200;height:180;overflow:auto;background:#2E7D32">
    <label Text="A" style="left:10;top:10;width:180;height:60;background:#FF9800"/>
    <label Text="S" Top="70" style="left:10;width:180;height:40;background:#2196F3;position:sticky;top:8px"/>
    <label Text="B" style="left:10;top:130;width:180;height:60;background:#FF9800"/>
    <label Text="C" style="left:10;top:210;width:180;height:60;background:#FF9800"/>
  </div>
</window>'''

u = ctypes.windll.user32
u.GetWindow.argtypes = [wintypes.HWND, ctypes.c_uint]
u.GetWindow.restype = wintypes.HWND
u.SendMessageW.argtypes = [wintypes.HWND, ctypes.c_uint, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
u.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
u.ScreenToClient.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.POINT)]
WM_MOUSEWHEEL = 0x020A


def children(h):
    out, c = [], u.GetWindow(h, 5)
    while c:
        out.append(c)
        c = u.GetWindow(c, 2)
    return out


def all_desc(h):
    out, stack = [], [h]
    while stack:
        for c in children(stack.pop()):
            out.append(c)
            stack.append(c)
    return out


def has_region(h):
    rgn = ctypes.windll.gdi32.CreateRectRgn(0, 0, 0, 0)
    n = u.GetWindowRgn(h, rgn)
    ctypes.windll.gdi32.DeleteObject(rgn)
    return n != 0


def rect(h):
    r = wintypes.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    return r.left, r.top, r.right - r.left, r.bottom - r.top


def rel_pos(container, h):
    x, y, w, hh = rect(h)
    pt = wintypes.POINT(x, y)
    u.ScreenToClient(container, ctypes.byref(pt))
    return pt.x, pt.y


def wheel(h, notches):
    delta = (notches * 120) & 0xFFFF
    u.SendMessageW(h, WM_MOUSEWHEEL, delta << 16, 0)


p = r'G:\PyArx\log\_t_sticky.xml'
open(p, 'w', encoding='utf-8').write(XML)
w = M.guiload(p, context={}, names={})
w.show()
M.pump(400)
top = wintypes.HWND(int(w.hwnd))
cont = [h for h in all_desc(top) if has_region(h)]
OK = True

ok0 = len(cont) == 1
OK &= ok0
print(('PASS ' if ok0 else 'FAIL ') + '找到 overflow 容器 %d 个' % len(cont))
if not ok0:
    print('t_sticky: FAIL')
    w.close()
    sys.exit(1)

kids = children(cont[0])
sticky = None
anchor = None
for h in kids:
    x, y, ww, hh = rect(h)
    if hh == 40:
        sticky = h
    elif anchor is None:
        anchor = h

ok1 = sticky is not None
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + 'position:sticky 子项已建出（高 40 的那个）')

y_sticky0 = rel_pos(cont[0], sticky)[1]
ok2 = y_sticky0 == 70
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + '未滚动时 sticky 停在正常流位置 y=70 (%d)' % y_sticky0)

wheel(anchor, -1)
M.pump(120)
y1 = rel_pos(cont[0], sticky)[1]
ok3 = y1 == 30
OK &= ok3
print(('PASS ' if ok3 else 'FAIL ') + '滚一格（40px）sticky 跟着上移到 y=30，还没到吸顶线 (%d)' % y1)

wheel(anchor, -1)
M.pump(120)
y2 = rel_pos(cont[0], sticky)[1]
ok4 = y2 == 8
OK &= ok4
print(('PASS ' if ok4 else 'FAIL ') + '滚两格（80px）sticky 越过 top:8px 后钉在 y=8 (%d)' % y2)

for _ in range(3):
    wheel(anchor, -1)
    M.pump(100)
y3 = rel_pos(cont[0], sticky)[1]
ok5 = y3 == 8
OK &= ok5
print(('PASS ' if ok5 else 'FAIL ') + '继续滚到底 sticky 仍钉在 y=8 (%d)' % y3)

for _ in range(6):
    wheel(anchor, 1)
    M.pump(100)
y4 = rel_pos(cont[0], sticky)[1]
ok6 = y4 == 70
OK &= ok6
print(('PASS ' if ok6 else 'FAIL ') + '滚回顶部 sticky 回到正常流位置 y=70 (%d)' % y4)

w.save_png(r'G:\PyArx\log\_t_sticky.png')
w.close()
print('t_sticky: %s' % ('PASS=6 FAIL=0' if OK else 'FAIL'))
sys.exit(0 if OK else 1)
