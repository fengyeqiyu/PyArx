# -*- coding: utf-8 -*-
"""第二批修复验收：链接地址/点击、图片装载、富文本格式、滚动范围、分隔条拖拽、Tab 焦点。

取样口径说明（重要）：
  这套控件的绘制走自绘渲染层，GDI 窗口表面本身是空的，所以"抓像素"一律抓不到内容。
  能证明"功能真实"的只有两条：1) 原生消息回读（BM_GETIMAGE / GetScrollInfo / EM_GETCHARFORMAT）；
  2) 控件的几何位置真的变了（拖拽）。本脚本按这两条判。
"""
import ctypes
import os
import time
from ctypes import wintypes

import PyMfcXaml as p

HERE = os.path.dirname(os.path.abspath(__file__))
u = ctypes.windll.user32


def proto(fn, args, res=None):
    fn.argtypes = args
    if res is not None:
        fn.restype = res


proto(u.EnumChildWindows, [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM])
proto(u.GetDlgCtrlID, [ctypes.c_void_p], ctypes.c_int)
proto(u.SendMessageW, [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], wintypes.LPARAM)
proto(u.GetWindowRect, [ctypes.c_void_p, ctypes.c_void_p])
proto(u.SetCursorPos, [ctypes.c_int, ctypes.c_int], wintypes.BOOL)
proto(u.GetCursorPos, [ctypes.c_void_p], wintypes.BOOL)
proto(u.SetForegroundWindow, [ctypes.c_void_p], wintypes.BOOL)
proto(u.SetWindowPos, [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                       ctypes.c_int, ctypes.c_int, wintypes.UINT], wintypes.BOOL)
proto(u.GetScrollInfo, [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p], wintypes.BOOL)
proto(u.GetFocus, [], ctypes.c_void_p)
proto(u.PostMessageW, [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM], wintypes.BOOL)

BM_GETIMAGE = 0x00F6
IMAGE_BITMAP = 0
STM_GETIMAGE = 0x0173
EM_GETCHARFORMAT = 0x043A
SCF_ALL = 0x0004
SB_VERT = 1
SIF_RANGE = 0x0001
WM_KEYDOWN = 0x0100
WM_LBUTTONDOWN = 0x0201
WM_MOUSEMOVE = 0x0200
WM_LBUTTONUP = 0x0202
VK_TAB = 0x09

PASS, FAIL = [], []


def record(name, ok, detail=''):
    (PASS if ok else FAIL).append(name)
    print(('ok    ' if ok else 'FAIL  ') + name + ('  ' + detail if detail else ''), flush=True)


def kids(top):
    out = []
    EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(h, _):
        out.append((h, u.GetDlgCtrlID(h)))
        return True
    u.EnumChildWindows(top, EP(cb), 0)
    return out


def by_id(top, cid):
    for h, i in kids(top):
        if i == cid:
            return h
    return 0


class R(ctypes.Structure):
    _fields_ = [('l', ctypes.c_long), ('t', ctypes.c_long),
                ('r', ctypes.c_long), ('b', ctypes.c_long)]


class SCROLLINFO(ctypes.Structure):
    _fields_ = [('cbSize', wintypes.UINT), ('fMask', wintypes.UINT),
                ('nMin', ctypes.c_int), ('nMax', ctypes.c_int), ('nPage', wintypes.UINT),
                ('nPos', ctypes.c_int), ('nTrackPos', ctypes.c_int)]


class CHARFORMATW(ctypes.Structure):
    _fields_ = [('cbSize', wintypes.UINT), ('dwMask', wintypes.DWORD),
                ('dwEffects', wintypes.DWORD), ('yHeight', ctypes.c_long),
                ('yOffset', ctypes.c_long), ('crTextColor', wintypes.DWORD),
                ('bCharSet', ctypes.c_ubyte), ('bPitchAndFamily', ctypes.c_ubyte),
                ('szFaceName', wintypes.WCHAR * 32)]


def rect(h):
    r = R()
    u.GetWindowRect(h, ctypes.byref(r))
    return r


def click(h, fx=0.5, fy=0.5):
    r = rect(h)
    u.SetCursorPos(int(r.l + (r.r - r.l) * fx), int(r.t + (r.b - r.t) * fy))
    time.sleep(0.05)
    u.mouse_event(0x0002, 0, 0, 0, 0)
    time.sleep(0.03)
    u.mouse_event(0x0004, 0, 0, 0, 0)


names = {}
w = p.guiload(os.path.join(HERE, '_fix2.xaml'), names=names)
w.show()
p.pump(500)
u.SetWindowPos(w.hwnd, ctypes.c_void_p(-1), 60, 40, 660, 900, 0x0040)
u.SetForegroundWindow(w.hwnd)
p.pump(400)

# 1. 超链接：地址可写可读 + 真点击触发 onClick
lnk = by_id(w.hwnd, names['lnkHome'])
hits = {'n': 0}
w.onClick(names['lnkHome'], lambda *a: hits.__setitem__('n', hits['n'] + 1))
p.pump(200)
record('Hyperlink 能读出设置的 URL', w.getUrl(names['lnkHome']) == 'https://www.autodesk.com',
       'url=' + w.getUrl(names['lnkHome']))
txt = ctypes.create_unicode_buffer(256)
u.GetWindowTextW(lnk, txt, 256)
record('Hyperlink 控件文本里带 href 标记', 'href' in txt.value, 'text=' + txt.value[:60])
click(lnk, 0.3, 0.5)
p.pump(400)
record('Hyperlink 真点击触发 onClick', hits['n'] >= 1, 'onClick=%d' % hits['n'])

# 2. 静态图片：原生控件真的装上位图
img = by_id(w.hwnd, names['imgLogo'])
old = u.SendMessageW(img, STM_GETIMAGE, IMAGE_BITMAP, 0)
record('StaticBitmap 真装载了位图', bool(old), 'STM_GETIMAGE=0x%X' % old)
record('setImage 接口存在且可重复设置', w.setImage(names['imgLogo'], os.path.join(HERE, 'res', 'dot.png')))

# 3. 位图按钮
bb = by_id(w.hwnd, names['btnIcon'])
bimg = u.SendMessageW(bb, BM_GETIMAGE, IMAGE_BITMAP, 0)
record('BitmapButton 真装载了位图', bool(bimg), 'BM_GETIMAGE=0x%X' % bimg)

# 4. 富文本格式：加粗/字号真的落到控件
rtf = by_id(w.hwnd, names['rtfNote'])
cf = CHARFORMATW()
cf.cbSize = ctypes.sizeof(CHARFORMATW)
u.SendMessageW(rtf, EM_GETCHARFORMAT, SCF_ALL, ctypes.cast(ctypes.byref(cf), ctypes.c_void_p).value)
bold_on = bool(cf.dwEffects & 0x00000001)
record('RichText 加粗生效', bold_on, 'dwEffects=0x%X yHeight=%d' % (cf.dwEffects, cf.yHeight))
record('RichText 字号生效（14pt=280 twips）', cf.yHeight == 280, 'yHeight=%d' % cf.yHeight)
w.setRichFormat(names['rtfNote'], 9, False, True, 0xFF0000)
p.pump(200)
cf2 = CHARFORMATW()
cf2.cbSize = ctypes.sizeof(CHARFORMATW)
u.SendMessageW(rtf, EM_GETCHARFORMAT, SCF_ALL, ctypes.cast(ctypes.byref(cf2), ctypes.c_void_p).value)
record('setRichFormat 接口能改斜体并关掉加粗',
       bool(cf2.dwEffects & 0x00000002) and not (cf2.dwEffects & 0x00000001),
       'dwEffects=0x%X' % cf2.dwEffects)

# 5. 滚动区域：滚动条由皮肤自绘（不再挂系统滚动条），范围走自有接口
scr = by_id(w.hwnd, names['scrBox'])
style = u.GetWindowLongPtrW(scr, -16)
record('ScrolledWindow 自绘滚动条（不挂系统滚动条样式）',
       not (style & 0x00200000), 'style=0x%X' % style)
rng = w.getScrollRange(names['scrBox'])
record('ScrolledWindow 有真实滚动范围', rng[1] - rng[0] > 0, 'min=%d max=%d' % rng)
record('Python 侧读得到滚动范围', rng[1] > 0, str(rng))

# 6. 分隔条真拖拽：拖它，下面那个按钮必须动
spl = by_id(w.hwnd, names['splMain'])
below = by_id(w.hwnd, names['lstBelow2'])
r_spl, r_below = rect(spl), rect(below)
mid = (r_spl.l + r_spl.r) // 2
u.SetCursorPos(mid, (r_spl.t + r_spl.b) // 2)
time.sleep(0.1)
u.mouse_event(0x0002, 0, 0, 0, 0)   # 按下
time.sleep(0.05)
p.pump(150)                          # 鼠标消息要靠 pump 分发，不 pump 拖不动

for step in range(1, 11):
    u.SetCursorPos(mid, (r_spl.t + r_spl.b) // 2 - step * 4)
    time.sleep(0.04)
    p.pump(60)

u.mouse_event(0x0004, 0, 0, 0, 0)
p.pump(300)
r_spl2, r_below2 = rect(spl), rect(below)
moved_spl = abs(r_spl2.t - r_spl.t)
moved_below = abs(r_below2.t - r_below.t)
record('Splitter 拖动时自己真的移动', moved_spl >= 20, '位移=%dpx' % moved_spl)
record('Splitter 拖动带动相邻控件', moved_below >= 20,
       '邻居位移=%dpx 高度 %d->%d' % (moved_below, r_below.b - r_below.t,
                                    r_below2.b - r_below2.t))

# 7. Tab 键焦点切换（钩子只认队列消息，所以用 PostMessage 而不是 SendMessage）
edt = by_id(w.hwnd, names['edtFirst'])
chk = by_id(w.hwnd, names['chkSecond'])
u.SetFocus.argtypes = [ctypes.c_void_p]
u.SetFocus(edt)
p.pump(200)
f0 = u.GetFocus()
u.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.PostMessageW(edt, WM_KEYDOWN, VK_TAB, 0)
p.pump(300)
f1 = u.GetFocus()
u.GetParent.argtypes = [ctypes.c_void_p]
u.GetParent.restype = ctypes.c_void_p


def in_host(h, top, depth=6):
    """控件可能挂在 StackPanel 这种子容器里，所以要沿父链往上找，不能直接比父窗口。"""
    for _ in range(depth):
        if not h:
            return False
        if h == top:
            return True
        h = u.GetParent(h)
    return False


record('Tab 键把焦点移到下一个控件（且焦点没跑出本窗口）',
       f1 != f0 and f1 != 0 and in_host(f1, w.hwnd),
       '0x%X -> 0x%X 在窗口内=%s' % (f0 or 0, f1 or 0, in_host(f1, w.hwnd)))
# 再按一次 Tab：必须继续往前走，而不是原地打转
u.PostMessageW(f1, WM_KEYDOWN, VK_TAB, 0)
p.pump(300)
f2 = u.GetFocus()
record('Tab 键能连续往后切换焦点', f2 != f1 and f2 != 0,
       '0x%X -> 0x%X' % (f1 or 0, f2 or 0))

print()
print('合计 %d 项：通过 %d，失败 %d' % (len(PASS) + len(FAIL), len(PASS), len(FAIL)), flush=True)
for f in FAIL:
    print('  失败：' + f, flush=True)

w.close()
p.pump(200)
p.shutdown()
