import os, sys, ctypes
from ctypes import wintypes
ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M

XML = '''<window Width="420" Height="300" Background="#313842">
  <div style="left:20;top:20;width:200;height:120;overflow:scroll;background:#2E7D32">
    <label Text="A" style="left:10;top:10;width:80;height:60;background:#FF9800"/>
    <label Text="B" style="left:10;top:90;width:80;height:60;background:#FF9800"/>
    <label Text="C" style="left:10;top:170;width:80;height:60;background:#FF9800"/>
    <label Text="F" style="left:110;top:20;width:60;height:30;background:#2196F3;position:fixed"/>
  </div>
</window>'''

u = ctypes.windll.user32
u.GetWindow.argtypes = [wintypes.HWND, ctypes.c_uint]
u.GetWindow.restype = wintypes.HWND
u.GetDlgCtrlID.argtypes = [wintypes.HWND]
u.GetDlgCtrlID.restype = ctypes.c_int
u.SendMessageW.argtypes = [wintypes.HWND, ctypes.c_uint, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
u.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
u.ScreenToClient.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.POINT)]
WM_MOUSEWHEEL = 0x020A


def children(h):
    out, c = [], u.GetWindow(h, 5)           # GW_CHILD
    while c:
        out.append(c)
        c = u.GetWindow(c, 2)                # GW_HWNDNEXT
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


def rel_pos(container, h):
    r = wintypes.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    pt = wintypes.POINT(r.left, r.top)
    u.ScreenToClient(container, ctypes.byref(pt))
    return pt.x, pt.y


def wheel(h, notches):
    delta = (notches * 120) & 0xFFFF
    u.SendMessageW(h, WM_MOUSEWHEEL, delta << 16, 0)


p = r'G:\PyArx\log\_t_ovs.xml'
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
    print('t_overflow_scroll: FAIL')
    w.close()
    sys.exit(1)

kids = children(cont[0])
kids.sort(key=lambda h: rel_pos(cont[0], h)[1])
anchor = kids[0]
fx, fy0 = None, None
for h in kids:
    px_, py_ = rel_pos(cont[0], h)
    if px_ > 100:                 # position:fixed 的那个（left:110）
        fx, fy0 = px_, py_
x0, y0 = rel_pos(cont[0], anchor)
cr = wintypes.RECT()
u.GetClientRect(cont[0], ctypes.byref(cr))
viewH = cr.bottom - cr.top

wheel(anchor, -1)
M.pump(120)
x1, y1 = rel_pos(cont[0], anchor)
ok1 = (y0 - y1 == 40) and (x1 == x0)
OK &= ok1
print(('PASS ' if ok1 else 'FAIL ') + '滚轮下滚一格：子项上移 40 且不横移 (y %d -> %d, viewH=%d)' % (y0, y1, viewH))

fy_mid = None
for h in kids:
    px_, py_ = rel_pos(cont[0], h)
    if px_ > 100:
        fy_mid = py_
ok_f = fy_mid == fy0
OK &= ok_f
print(('PASS ' if ok_f else 'FAIL ') + 'CSS position:fixed 子项不随容器滚动 (y=%s 期望=%s)' % (fy_mid, fy0))

for _ in range(4):
    wheel(anchor, -1)
    M.pump(80)
y2 = rel_pos(cont[0], anchor)[1]
maxoff = max(0, 230 - viewH)             # 内容 bottom = 170+60
ok2 = y2 == y0 - maxoff
OK &= ok2
print(('PASS ' if ok2 else 'FAIL ') + '滚到底 clamp 在内容末 (y=%d 期望=%d)' % (y2, y0 - maxoff))

wheel(anchor, 1)
M.pump(120)
y3 = rel_pos(cont[0], anchor)[1]
ok3 = y3 == y2 + 40
OK &= ok3
print(('PASS ' if ok3 else 'FAIL ') + '反向滚一格回退 40 (%d -> %d)' % (y2, y3))

for _ in range(5):
    wheel(anchor, 1)
    M.pump(80)
y4 = rel_pos(cont[0], anchor)[1]
ok4 = y4 == y0
OK &= ok4
print(('PASS ' if ok4 else 'FAIL ') + '滚回顶部停在 0 偏移 (y=%d 期望=%d)' % (y4, y0))

ok5 = fx is not None and fy0 is not None
OK &= ok5
print(('PASS ' if ok5 else 'FAIL ') + 'position:fixed 子项被识别 (x=%s y=%s)' % (fx, fy0))

w.save_png(r'G:\PyArx\log\_t_ovs.png')
w.close()
print('t_overflow_scroll: %s' % ('PASS=7 FAIL=0' if OK else 'FAIL'))
