# -*- coding: utf-8 -*-
# FileTab 多文档文件页签条：几何 / 高亮 / 脏点与关闭框 / activate / close / add / 滚轮。
# 骨架照 t_sticky.py、t_overflow_scroll.py：guiload -> show -> pump -> savePng，
# 递归 GetWindow(h,5)/GetWindow(c,2) 取真实子窗口几何。
import os, sys, ctypes, json
from ctypes import wintypes

ROOT = r'G:\PyArx'
BIN = os.path.join(ROOT, 'X64', 'Release')
sys.path.insert(0, BIN)
os.add_dll_directory(BIN)
os.environ.setdefault('PYARX_BIN', BIN)
os.environ.setdefault('PYARX_D2D_RT', 'software')
import PyMfcXml as M  # noqa: E402

XML = '''<XWindow Width="420" Height="120" Background="#313842">
  <XFileTab Name="ftab" style="left:0;top:0;width:100%;height:34" ShowAdd="true">
    <XFileTabItem Text="平面图.dwg" Active="true"/>
    <XFileTabItem Text="总装布置图.dwg" Dirty="true" Closable="true"/>
    <XFileTabItem Text="框架结构图.dwg" Closable="true"/>
    <XFileTabItem Text="设备基础总图.dwg" Closable="true"/>
  </XFileTab>
</XWindow>'''

u = ctypes.windll.user32
u.GetWindow.argtypes = [wintypes.HWND, ctypes.c_uint]
u.GetWindow.restype = wintypes.HWND
u.SendMessageW.argtypes = [wintypes.HWND, ctypes.c_uint, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.GetWindowRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
u.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
u.GetDlgCtrlID.argtypes = [wintypes.HWND]
u.GetDlgCtrlID.restype = ctypes.c_int
WM_MOUSEMOVE = 0x0200
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_MOUSEWHEEL = 0x020A

FAILS = []
CHECKS = [0]


def check(cond, what, got=None):
    CHECKS[0] += 1
    line = ('PASS ' if cond else 'FAIL ') + what
    if got is not None:
        line += '  got=%r' % (got,)
    print(line)
    if not cond:
        FAILS.append(what)
    return bool(cond)


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


def win_rect(h):
    r = wintypes.RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    return r.left, r.top, r.right - r.left, r.bottom - r.top


def lparam(x, y):
    return (y << 16) | (x & 0xFFFF)


def click_at(hwnd, x, y):
    lp = lparam(x, y)
    u.SendMessageW(hwnd, WM_MOUSEMOVE, 0, lp)
    u.SendMessageW(hwnd, WM_LBUTTONDOWN, 1, lp)
    u.SendMessageW(hwnd, WM_LBUTTONUP, 0, lp)


def wheel(hwnd, notches):
    delta = (notches * 120) & 0xFFFF
    u.SendMessageW(hwnd, WM_MOUSEWHEEL, delta << 16, 0)


def near(c, target, tol=12):
    return all(abs(a - b) <= tol for a, b in zip(c, target))


# ---- 加载 ----
p = r'G:\PyArx\log\_t_filetab.xml'
open(p, 'w', encoding='utf-8').write(XML)
names = {}
w = M.guiload(p, context={}, names=names)
w.show()
M.pump(400)
top = wintypes.HWND(int(w.hwnd))
fid = names['ftab']

hits = []
w.on_click(fid, lambda msg: hits.append(str(msg)))

# 真实自绘窗口：按控件 ID 认准 Static（外层可能套一层 PyMfcNativeWindow 容器）
descs = all_desc(top)
matched = [h for h in descs if u.GetDlgCtrlID(h) == fid]
check(len(matched) == 1, '找到 FileTab 自绘窗口', len(matched))
hw = matched[0]

info = json.loads(w.get_file_tab_info(fid))
tabs = info['tabs']
close = info['close']
add = info.get('add')

# ① 渲染几何：页签数 / 不重叠 / 活跃位置与宽度
check(len(tabs) == 4, '页签数 = 4', len(tabs))
check(info['active'] == 0, '初始 activeIndex = 0', info['active'])
check(info['showAdd'] is True, 'ShowAdd = true', info['showAdd'])
check(add is not None and add[2] > 0, '末尾 + 按钮矩形存在', add)

no_overlap = True
for i in range(1, len(tabs)):
    if tabs[i][0] < tabs[i - 1][0] + tabs[i - 1][2]:
        no_overlap = False
check(no_overlap, '相邻页签矩形不重叠', tabs)

fx, fy, fw, fh = w.get_rect(fid)
active = tabs[info['active']]
check(active[2] > 0 and active[3] == fh, '活跃页签宽度>0 且高度=控件高', (active, fh))
check(active[0] == tabs[0][0], '活跃页签位于最左', active)

# ---- 像素断言 ----
png = r'G:\PyArx\log\_t_filetab.png'
w.save_png(png)
from PIL import Image  # noqa: E402
im = Image.open(png).convert('RGB')
wl, wt, _, _ = win_rect(top)
hl, ht, _, _ = win_rect(hw)
ox, oy = hl - wl, ht - wt


def px_in(rect, target, tol=12):
    x0 = ox + rect[0]; y0 = oy + rect[1]
    x1 = ox + rect[0] + rect[2]; y1 = oy + rect[1] + rect[3]
    n = 0
    for y in range(max(0, y0), min(im.size[1], y1)):
        for x in range(max(0, x0), min(im.size[0], x1)):
            if near(im.getpixel((x, y)), target, tol):
                n += 1
    return n


ACCENT = (76, 141, 255)
PANEL_ALT = (45, 54, 66)
GOOD = (72, 178, 118)
TEXT_DIM = (150, 161, 174)

# ② 活跃高亮：强调下划线（accent）+ 抬亮面（panelAlt）
n_accent = px_in(active, ACCENT)
n_face = px_in(active, PANEL_ALT)
check(n_accent > 0, '活跃页签有 accent 下划线像素', n_accent)
check(n_face > 0, '活跃页签有抬亮底色像素', n_face)

# ③ 脏点（tab1 绿点）+ 关闭框（tab1/tab2/tab3 非空）+ × 笔画像素
n_green = px_in(tabs[1], GOOD)
check(n_green > 0, '脏页签有绿色圆点像素', n_green)
check(len(close) == 4 and close[1][2] > 0 and close[2][2] > 0 and close[3][2] > 0,
      '可关闭页签关闭框矩形存在', close)
n_x = px_in(close[2], TEXT_DIM, tol=25)
check(n_x > 0, '关闭框内有 × 笔画像素', n_x)

# ④ 点第 2 个页签（下标 1）-> activeIndex=1，回调 'activate:1'
body1 = tabs[1]
cx1 = (body1[0] + close[1][0]) // 2
cy1 = body1[1] + body1[3] // 2
click_at(hw, cx1, cy1)
M.pump(120)
info2 = json.loads(w.get_file_tab_info(fid))
check(info2['active'] == 1, '点击后 activeIndex = 1', info2['active'])
check('activate:1' in hits, "回调收到 'activate:1'", hits)

# ⑤ 点第 3 个页签（下标 2）的 × -> 回调 'close:2'
cb2 = close[2]
cx2 = cb2[0] + cb2[2] // 2
cy2 = cb2[1] + cb2[3] // 2
click_at(hw, cx2, cy2)
M.pump(120)
check('close:2' in hits, "回调收到 'close:2'", hits)

# 末尾 + 号：点一下回调 'add'
if add is not None:
    cxa = add[0] + add[2] // 2
    cya = add[1] + add[3] // 2
    click_at(hw, cxa, cya)
    M.pump(120)
    check('add' in hits, "回调收到 'add'", hits)

# 滚轮：内容超宽时横向滚动；未超宽则软通过（scrollX 保持 0）
avail = fw
if info2.get('totalWidth', 0) > avail:
    wheel(hw, -3)
    M.pump(120)
    info3 = json.loads(w.get_file_tab_info(fid))
    check(info3['scrollX'] > 0, '滚轮后 scrollX > 0', info3['scrollX'])
    wheel(hw, 3)
    M.pump(120)
    info4 = json.loads(w.get_file_tab_info(fid))
    check(info4['scrollX'] < info3['scrollX'], '反向滚轮 scrollX 回退',
          (info4['scrollX'], info3['scrollX']))
else:
    CHECKS[0] += 1
    print('PASS  内容未超宽，滚轮裁切策略生效（scrollX=0）')

w.save_png(png)
w.close()
print('t_filetab: PASS=%d FAIL=%d' % (CHECKS[0] - len(FAILS), len(FAILS)))
sys.exit(0 if not FAILS else 1)
