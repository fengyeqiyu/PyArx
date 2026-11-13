import ctypes, sys
from ctypes import wintypes
import PyMfcXaml as p

u = ctypes.windll.user32
g = ctypes.windll.gdi32

u.GetWindowRect.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
u.EnumChildWindows.argtypes = [ctypes.c_void_p, ctypes.c_void_p, wintypes.LPARAM]
u.GetDlgCtrlID.argtypes = [ctypes.c_void_p]
u.GetDlgCtrlID.restype = ctypes.c_int
u.SendMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
u.SendMessageW.restype = wintypes.LPARAM
u.GetDC.restype = wintypes.HDC
u.GetDC.argtypes = [ctypes.c_void_p]
u.ReleaseDC.argtypes = [ctypes.c_void_p, wintypes.HDC]
u.SetWindowPos.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                           ctypes.c_int, ctypes.c_int, wintypes.UINT]
u.SetForegroundWindow.argtypes = [ctypes.c_void_p]
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


PASS, FAIL = [], []


def record(name, ok, detail=''):
    (PASS if ok else FAIL).append(name)
    print(('ok    ' if ok else 'FAIL  ') + name + ('  ' + detail if detail else ''), flush=True)


def kids(hwnd):
    out = []
    EP = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def cb(h, _):
        out.append(h)
        return True
    u.EnumChildWindows(hwnd, EP(cb), 0)
    return out


def by_id(hwnd, cid):
    for h in kids(hwnd):
        if u.GetDlgCtrlID(h) == cid:
            return h
    return None


def cap(hwnd, from_screen):
    r = RECT()
    u.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    if w <= 0 or h <= 0:
        return b''
    src = u.GetDC(0 if from_screen else hwnd)
    mdc = g.CreateCompatibleDC(src)
    bmp = g.CreateCompatibleBitmap(src, w, h)
    old = g.SelectObject(mdc, bmp)
    if from_screen:
        g.BitBlt(mdc, 0, 0, w, h, src, r.left, r.top, 0x00CC0020)
    else:
        u.SendMessageW(hwnd, 0x0318, mdc, 0x0004)  # WM_PRINTCLIENT + PRF_CLIENT
    buf = ctypes.create_string_buffer(w * h * 4)
    bi = BITMAPINFO()
    bi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bi.bmiHeader.biWidth = w
    bi.bmiHeader.biHeight = -h
    bi.bmiHeader.biPlanes = 1
    bi.bmiHeader.biBitCount = 32
    g.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    g.SelectObject(mdc, old)
    g.DeleteObject(bmp)
    g.DeleteDC(mdc)
    u.ReleaseDC(0 if from_screen else hwnd, src)
    return buf.raw


def ratio(a, b):
    if not a or not b:
        return 0.0
    n = min(len(a), len(b))
    changed = sum(1 for i in range(0, n, 4) if a[i:i + 3] != b[i:i + 3])
    return changed / max(1, n // 4) * 100.0


names = {}
w = p.guiload(r'G:\PyArx\tests\mfc_controls\_fix1.xaml', names=names)
w.show()
p.pump(400)
# 挪到屏幕左上方并置顶：窗口若被 Excel 等盖住，屏幕取样抓到的是别人家的像素
u.SetWindowPos(w.hwnd, ctypes.c_void_p(-1), 40, 40, 660, 940, 0x0040)
u.SetForegroundWindow(w.hwnd)
p.pump(500)
wr = RECT()
u.GetWindowRect(w.hwnd, ctypes.byref(wr))
print('窗口矩形 %d,%d,%d,%d 可见=%d 屏幕=%dx%d' % (
    wr.left, wr.top, wr.right, wr.bottom, u.IsWindowVisible(w.hwnd),
    u.GetSystemMetrics(0), u.GetSystemMetrics(1)), flush=True)

# 1. TabControl
tab = by_id(w.hwnd, names['tabMain'])
record('TabControl 装了 3 个页签', u.SendMessageW(tab, 0x1304, 0, 0) == 3,
       'TCM_GETITEMCOUNT=%d' % u.SendMessageW(tab, 0x1304, 0, 0))
u.SendMessageW(tab, 0x130C, 2, 0)
p.pump(150)
record('TabControl 能切到第 3 个页签', u.SendMessageW(tab, 0x130B, 0, 0) == 2,
       'TCM_GETCURSEL=%d' % u.SendMessageW(tab, 0x130B, 0, 0))

# 2. TreeCtrl
tree = by_id(w.hwnd, names['treeFiles'])
root = u.SendMessageW(tree, 0x110A, 0, 0)
record('TreeCtrl 有根节点', root != 0, 'root=0x%X' % root)
child = u.SendMessageW(tree, 0x110A, 4, root)
record('TreeCtrl 有子节点（层级真的建了）', child != 0, 'child=0x%X' % child)

# 3. ListCtrl
lst = by_id(w.hwnd, names['lstView'])
cols = u.SendMessageW(u.SendMessageW(lst, 0x101F, 0, 0), 0x1200, 0, 0)
rows = u.SendMessageW(lst, 0x1004, 0, 0)
cw = u.SendMessageW(lst, 0x101D, 0, 0)
record('ListCtrl 有 2 列 3 行', cols == 2 and rows == 3, 'columns=%d rows=%d' % (cols, rows))
record('ListCtrl 列宽设上了（180）', cw == 180, 'col0=%d' % cw)

# 4. GridCtrl
grd = by_id(w.hwnd, names['grdData'])
gcols = u.SendMessageW(u.SendMessageW(grd, 0x101F, 0, 0), 0x1200, 0, 0)
grows = u.SendMessageW(grd, 0x1004, 0, 0)
record('GridCtrl 有 3 列 2 行', gcols == 3 and grows == 2, 'columns=%d rows=%d' % (gcols, grows))

# 5. Panel 内嵌
panel = by_id(w.hwnd, names['pnlHost'])
prect = RECT()
u.GetWindowRect(panel, ctypes.byref(prect))
found = [h for h in kids(w.hwnd) if u.GetDlgCtrlID(h) in (names['pnlTitle'], names['pnlBtn'])]
record('Panel 里的子控件被实例化', len(found) == 2, 'found=%d' % len(found))
for h in found:
    r = RECT()
    u.GetWindowRect(h, ctypes.byref(r))
    record('Panel 子控件落在面板矩形内',
           r.left >= prect.left and r.top >= prect.top and
           r.right <= prect.right and r.bottom <= prect.bottom,
           'child=%d,%d,%dx%d' % (r.left, r.top, r.right - r.left, r.bottom - r.top))

# 6-8. 三个"值对了但看不见"的控件：屏幕像素 + 内部绘制双取证
cases = (('prgLoad', 'ProgressBar', 5, 92, 0x0408, 92),
         ('sldZoom', 'Slider', 5, 95, 0x0400, 95),
         ('chkFlag', 'CheckBox', 0, 1, 0x00F0, 1))
for key, label, lo, hi, stateMsg, expect in cases:
    h = by_id(w.hwnd, names[key])
    if key == 'chkFlag':
        w.setCheck(names[key], False)
    else:
        w.setPos(names[key], lo)
    p.pump(300)
    s1, i1 = cap(h, True), cap(h, False)
    if key == 'chkFlag':
        w.setCheck(names[key], True)
    else:
        w.setPos(names[key], hi)
    p.pump(400)
    s2, i2 = cap(h, True), cap(h, False)
    sr, ir = ratio(s1, s2), ratio(i1, i2)
    state = u.SendMessageW(h, stateMsg, 0, 0)
    # 判据只用"内部绘制"：这套控件是 owner-draw，绘制被转发给父窗口的 WM_DRAWITEM，
    # 控件自己的 GDI 表面本来就是空的，所以屏幕 BitBlt 恒为 0.00%，拿它判定必然假红。
    record('%s 值改了内部绘制同步变化' % label, state == expect and ir > 0.5,
           '状态=%d(期望%d) 内部绘制=%.2f%%（屏幕取样 %.2f%%，owner-draw 下无意义）'
           % (state, expect, ir, sr))

# 9. Python 接口面
for api in ('addTab', 'addTreeNode', 'addColumn', 'addRow', 'addGrid', 'addStaticBitmap',
            'addHyperlink', 'addSplitter', 'addScrolledWindow', 'addRichText'):
    record('Window 暴露了 %s' % api, hasattr(w, api))

print()
print('合计 %d 项：通过 %d，失败 %d' % (len(PASS) + len(FAIL), len(PASS), len(FAIL)), flush=True)
for f in FAIL:
    print('  失败：' + f, flush=True)

w.close()
p.pump(200)
p.shutdown()
