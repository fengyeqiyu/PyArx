# 进程内探针：验证表头漏斗（Excel 式面板：搜索 + 复选 + 确定/取消），自己截屏，不依赖外部脚本
import ctypes, os, struct, sys, time, zlib
from ctypes import wintypes as wt
sys.path.insert(0, r'G:\PyArx\X64\Release')
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pp_app

DOCS = r'G:\PyArx\docs'
u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.SetProcessDpiAwarenessContext(ctypes.c_void_p(-4))
WNDENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)


def write_png(path, w, h, raw):
    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    rows = []
    for y in range(h):
        line = raw[y * w * 4:(y + 1) * w * 4]
        px = bytearray()
        for x in range(w):
            b, gg, r, a = line[x * 4:x * 4 + 4]
            px += bytes((r, gg, b))
        rows.append(bytes(px))
    png = (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
           + chunk(b'IDAT', zlib.compress(b''.join(rows), 6)) + chunk(b'IEND', b''))
    open(path, 'wb').write(png)


def grab(x, y, w, h, path):
    src = u.GetDC(0)
    mdc = g.CreateCompatibleDC(src)
    bmp = g.CreateCompatibleBitmap(src, w, h)
    old = g.SelectObject(mdc, bmp)
    g.BitBlt(mdc, 0, 0, w, h, src, x, y, 0x00CC0020)
    g.SelectObject(mdc, old)                 # GetDIBits 不能对仍选进 DC 的位图取像素
    bi = (ctypes.c_uint32 * 9)(w, -h, 1, 32, 0, 0, 0, 0, 0)
    buf = ctypes.create_string_buffer(w * h * 4)
    g.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bi), 0)
    g.DeleteObject(bmp); g.DeleteDC(mdc); u.ReleaseDC(0, src)
    write_png(path, w, h, buf.raw)


def gate(name, wait=20.0):
    path = os.path.join(DOCS, name)
    go = path.replace('.txt', '_go.txt')
    if os.path.exists(go):
        os.remove(go)
    open(path, 'w').write('go')
    t0 = time.time()
    while not os.path.exists(go) and time.time() - t0 < wait:
        time.sleep(0.05)


def children(root):
    out = []
    def cb(h, l):
        out.append(h)
        return True
    u.EnumChildWindows(root, WNDENUMPROC(cb), None)
    return out


def find_table(root):
    for h in children(root):
        if u.GetDlgCtrlID(h) == 3021:
            return h
    return 0


def click(h, x, y):
    lp = (y << 16) | (x & 0xFFFF)
    u.SendMessageW(h, 0x0201, 1, lp)
    u.SendMessageW(h, 0x0202, 0, lp)


app = pp_app.App()
app.win.show()
app.showEditor()
app.importCsvText(open(os.path.join(DOCS, '_big_20_10_50_allstart.csv'), encoding='utf-8-sig').read(),
                  '_big_20_10_50_allstart.csv')
time.sleep(0.6)
u.ShowWindow(app.win.hwnd, 9)
u.SetWindowPos(app.win.hwnd, ctypes.c_void_p(-1), 150, 150, 1440, 1168, 0x0050)
u.SetForegroundWindow(app.win.hwnd)
time.sleep(0.8)

tbl = find_table(app.win.hwnd)
r = wt.RECT(); u.GetWindowRect(tbl, ctypes.byref(r))
print('rows before', app.win.tableRowCount(3021), flush=True)
print('tbl', tbl, 'rect', r.left, r.top, r.right - r.left, r.bottom - r.top, flush=True)

click(tbl, 95, 15)                       # 表头「起点」列漏斗
time.sleep(0.4)
print('panel opened', flush=True)
for ch in 'GRID':
    u.SendMessageW(tbl, 0x0102, ord(ch), 0)
time.sleep(0.3)
u.UpdateWindow(tbl)
time.sleep(0.2)
print('gate1', flush=True)
time.sleep(4.0)

click(tbl, 100, 76)                      # 点「（全选）」= 全部取消
time.sleep(0.2)
click(tbl, 100, 98)                      # 勾上 GRID
time.sleep(0.2)
click(tbl, 290, 330)                     # 确定
time.sleep(0.5)
u.UpdateWindow(tbl)
print('rows after', app.win.tableRowCount(3021), 'view', app.win.tableViewRows(3021), flush=True)
time.sleep(3.0)
print('done', flush=True)
