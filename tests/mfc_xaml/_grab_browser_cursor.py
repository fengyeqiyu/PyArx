# -*- coding: utf-8 -*-
"""从浏览器实际使用的 HCURSOR 句柄里抓光标位图：开一个测试页，
把鼠标放到各 cursor 样式的格子上，读 GetCursorInfo 拿 hCursor，
再 GetIconInfo + DrawIconEx 导出位图和热区，并写成 .cur。"""
import ctypes
import os
import struct
import subprocess
import time
import zlib
from ctypes import wintypes as wt

OUT_DIR = r'G:\PyArx\docs\_browser_cursors'
HTML = r'G:\PyArx\docs\_cursor_probe.html'
EDGE = r'C:\Program Files (x86)\Microsoft\Edge\Application\153.0.0.0'
IMG_CURSOR = 2
LR_LOADFROMFILE = 0x10
DI_NORMAL = 3

ROWS = ['grab', 'grabbing', 'default', 'pointer', 'move', 'not-allowed', 'zoom-in', 'text', 'nesw-resize']

PAGE = """<!doctype html><html><head><meta charset="utf-8"><style>
html,body{margin:0;padding:0;font:14px sans-serif;background:#fff}
.row{height:%dpx;border-bottom:1px solid #ddd;display:flex;align-items:center;padding-left:20px}
</style></head><body>
%s
</body></html>""" % (0, '')


def find_edge():
    base = r'C:\Program Files (x86)\Microsoft\Edge\Application'
    if not os.path.isdir(base):
        return None
    vers = sorted(os.listdir(base))
    for v in reversed(vers):
        exe = os.path.join(base, v, 'msedge.exe')
        if os.path.exists(exe):
            return exe
    return None


def build_html(row_h=96):
    rows = ''.join('<div class="row" style="cursor:%s">cursor: %s</div>' % (c, c) for c in ROWS)
    html = """<!doctype html><html><head><meta charset="utf-8"><style>
html,body{margin:0;padding:0;font:14px sans-serif;background:#fff}
.row{height:%dpx;border-bottom:1px solid #ddd;display:flex;align-items:center;padding-left:20px}
</style></head><body>%s</body></html>""" % (row_h, rows)
    with open(HTML, 'w', encoding='utf-8') as f:
        f.write(html)
    return row_h


class CURSORINFO(ctypes.Structure):
    _fields_ = [('cbSize', wt.DWORD), ('flags', wt.DWORD),
                ('hCursor', wt.HANDLE), ('ptScreenPos', wt.POINT)]


class ICONINFO(ctypes.Structure):
    _fields_ = [('fIcon', wt.BOOL), ('xHotspot', wt.DWORD), ('yHotspot', wt.DWORD),
                ('hbmMask', wt.HBITMAP), ('hbmColor', wt.HBITMAP)]


class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG),
                ('biPlanes', wt.WORD), ('biBitCount', wt.WORD), ('biCompression', wt.DWORD),
                ('biSizeImage', wt.DWORD), ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG),
                ('biClrUsed', wt.DWORD), ('biClrImportant', wt.DWORD)]


u = ctypes.windll.user32
g = ctypes.windll.gdi32
u.GetCursorInfo.argtypes = [ctypes.c_void_p]
u.GetCursorInfo.restype = wt.BOOL
u.GetIconInfo.argtypes = [wt.HANDLE, ctypes.c_void_p]
u.GetIconInfo.restype = wt.BOOL
u.DrawIconEx.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int, wt.HANDLE, ctypes.c_int, ctypes.c_int,
                         wt.UINT, wt.HANDLE, wt.UINT]
u.LoadCursorW.argtypes = [wt.HINSTANCE, ctypes.c_void_p]
u.LoadCursorW.restype = wt.HANDLE
u.SetCursorPos.argtypes = [ctypes.c_int, ctypes.c_int]
u.GetWindowRect.argtypes = [wt.HWND, ctypes.c_void_p]
WNDENUMPROC = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
u.EnumWindows.argtypes = [WNDENUMPROC, wt.LPARAM]
u.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.c_void_p]
u.GetWindowTextW.argtypes = [wt.HWND, ctypes.c_wchar_p, ctypes.c_int]
u.IsWindowVisible.argtypes = [wt.HWND]
g.CreateDIBSection.restype = wt.HANDLE
g.CreateDIBSection.argtypes = [wt.HDC, ctypes.c_void_p, wt.UINT, ctypes.c_void_p, wt.HANDLE, wt.UINT]
g.CreateCompatibleDC.restype = wt.HANDLE
g.CreateCompatibleDC.argtypes = [wt.HDC]
g.SelectObject.argtypes = [wt.HDC, wt.HANDLE]
g.SelectObject.restype = wt.HANDLE
g.CreateSolidBrush.restype = wt.HANDLE
g.CreateSolidBrush.argtypes = [wt.COLORREF]
u.FillRect.argtypes = [wt.HDC, ctypes.c_void_p, wt.HANDLE]
g.DeleteObject.argtypes = [wt.HANDLE]
g.DeleteDC.argtypes = [wt.HDC]
g.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, wt.UINT, wt.UINT, ctypes.c_void_p, ctypes.c_void_p, wt.UINT]
g.GetDIBits.restype = ctypes.c_int
u.mouse_event.argtypes = [wt.DWORD, wt.DWORD, wt.DWORD, wt.DWORD, wt.DWORD]


def save_png(path, w, h, rows):
    raw = bytearray()
    for y in range(h):
        raw += b'\x00' + bytes(rows[y])

    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = (b'\x89PNG\r\n\x1a\n' +
           chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
           chunk(b'IDAT', zlib.compress(bytes(raw), 9)) +
           chunk(b'IEND', b''))
    with open(path, 'wb') as f:
        f.write(png)


def render(hcur, size, scale):
    bih = BITMAPINFOHEADER(40, size, -size, 1, 32, 0, size * size * 4, 0, 0, 0, 0)
    bits = ctypes.c_void_p()
    hbm = g.CreateDIBSection(0, ctypes.byref(bih), 0, ctypes.byref(bits), None, 0)
    mem = g.CreateCompatibleDC(0)
    g.SelectObject(mem, hbm)
    br = g.CreateSolidBrush(0x00E6E6E6)
    r = wt.RECT(0, 0, size, size)
    u.FillRect(mem, ctypes.byref(r), br)
    ok = u.DrawIconEx(mem, 0, 0, hcur, size, size, 0, None, DI_NORMAL)
    buf = (ctypes.c_ubyte * (size * size * 4)).from_address(bits.value)
    rows = []
    for y in range(size * scale):
        sy = y // scale
        row = bytearray()
        for x in range(size * scale):
            sx = x // scale
            i = (sy * size + sx) * 4
            row += bytes((buf[i + 2], buf[i + 1], buf[i]))
        rows.append(row)
    g.DeleteObject(br)
    g.DeleteObject(hbm)
    g.DeleteDC(mem)
    return ok, rows


def get_cursor():
    ci = CURSORINFO()
    ci.cbSize = ctypes.sizeof(CURSORINFO)
    u.GetCursorInfo(ctypes.byref(ci))
    return ci.hCursor


def image_bytes(px, size):
    xor = bytearray()
    for y in range(size - 1, -1, -1):
        for x in range(size):
            r_, g_, b_, a_ = px[y][x]
            xor += bytes((b_, g_, r_, a_))
    rowbytes = ((size + 31) // 32) * 4
    andmask = bytearray()
    for y in range(size - 1, -1, -1):
        row = bytearray(rowbytes)
        for x in range(size):
            if px[y][x][3] == 0:
                row[x // 8] |= 0x80 >> (x % 8)
        andmask += row
    bi = struct.pack('<IiiHHI', 40, size, size * 2, 1, 32, 0)
    bi += struct.pack('<IiiII', len(xor) + len(andmask), 0, 0, 0, 0)
    return bi + bytes(xor) + bytes(andmask)


def render_raw(hcur, size, bg):
    """把光标画到纯 bg 底色上，返回每像素 (b,g,r)。"""
    bih = BITMAPINFOHEADER(40, size, -size, 1, 32, 0, size * size * 4, 0, 0, 0, 0)
    bits = ctypes.c_void_p()
    hbm = g.CreateDIBSection(0, ctypes.byref(bih), 0, ctypes.byref(bits), None, 0)
    mem = g.CreateCompatibleDC(0)
    g.SelectObject(mem, hbm)
    br = g.CreateSolidBrush(bg)
    r = wt.RECT(0, 0, size, size)
    u.FillRect(mem, ctypes.byref(r), br)
    u.DrawIconEx(mem, 0, 0, hcur, size, size, 0, None, DI_NORMAL)
    buf = (ctypes.c_ubyte * (size * size * 4)).from_address(bits.value)
    out = [tuple(buf[i * 4:i * 4 + 4]) for i in range(size * size)]
    g.DeleteObject(br)
    g.DeleteObject(hbm)
    g.DeleteDC(mem)
    return out


def dump_cur(hcur, path):
    """黑白两底求 alpha：alpha = 255 - (白底 - 黑底)，颜色 = 黑底 / alpha。"""
    ii = ICONINFO()
    if not u.GetIconInfo(hcur, ctypes.byref(ii)):
        return None
    on_w = render_raw(hcur, 32, 0x00FFFFFF)
    on_b = render_raw(hcur, 32, 0x00000000)
    px = [[(0, 0, 0, 0)] * 32 for _ in range(32)]
    for y in range(32):
        for x in range(32):
            i = y * 32 + x
            bw, gw, rw, _ = on_w[i]
            bb, gb, rb, _ = on_b[i]
            a = 255 - ((abs(rw - rb) + abs(gw - gb) + abs(bw - bb)) // 3)
            a = max(0, min(255, a))
            if a < 4:
                px[y][x] = (0, 0, 0, 0)
                continue
            # 黑底像素 = 颜色 * (a/255)
            k = 255.0 / a
            px[y][x] = (min(255, int(rb * k)), min(255, int(gb * k)),
                        min(255, int(bb * k)), a)
    body = image_bytes([[tuple(p) for p in row] for row in px], 32)
    hdr = struct.pack('<HHH', 0, 2, 1)
    hdr += struct.pack('<BBBBHHII', 32, 32, 0, 0, ii.xHotspot & 0xFFFF, ii.yHotspot & 0xFFFF,
                       len(body), 22)
    with open(path, 'wb') as f:
        f.write(hdr + body)
    return ii.xHotspot, ii.yHotspot


_cursor_probe = '_cursor_probe'


def all_windows():
    hits = []

    def cb(hwnd, _):
        if u.IsWindowVisible(hwnd):
            t = ctypes.create_unicode_buffer(256)
            u.GetWindowTextW(hwnd, t, 256)
            if t.value:
                r = wt.RECT()
                u.GetWindowRect(hwnd, ctypes.byref(r))
                hits.append((hwnd, t.value, r))
        return True

    u.EnumWindows(WNDENUMPROC(cb), 0)
    return hits


def windows_for_pid(pid):
    hits = []

    def cb(hwnd, _):
        p = wt.DWORD()
        u.GetWindowThreadProcessId(hwnd, ctypes.byref(p))
        if p.value == pid and u.IsWindowVisible(hwnd):
            t = ctypes.create_unicode_buffer(256)
            u.GetWindowTextW(hwnd, t, 256)
            r = wt.RECT()
            u.GetWindowRect(hwnd, ctypes.byref(r))
            hits.append((hwnd, t.value, r))
        return True

    u.EnumWindows(WNDENUMPROC(cb), 0)
    return hits


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    row_h = build_html()
    exe = find_edge()
    if not exe:
        raise SystemExit('no edge')
    profile = r'G:\PyArx\docs\_probe_profile'
    proc = subprocess.Popen([exe, '--user-data-dir=' + profile, '--new-window',
                             '--window-size=900,1000', '--window-position=40,40',
                             '--no-first-run', '--disable-sync', 'file:///' + HTML.replace('\\', '/')])
    time.sleep(4)
    wins = windows_for_pid(proc.pid)
    if not wins:
        time.sleep(4)
        wins = [w for w in all_windows() if _cursor_probe in w[1]]
    if not wins:
        raise SystemExit('no window for pid %d' % proc.pid)
    hwnd, title, rect = wins[0]
    print('window', title, rect.left, rect.top, rect.right, rect.bottom)
    u.SetForegroundWindow(hwnd)
    time.sleep(1)
    cr = wt.RECT()
    u.GetClientRect.argtypes = [wt.HWND, ctypes.c_void_p]
    u.GetClientRect(hwnd, ctypes.byref(cr))
    org = wt.POINT(0, 0)
    u.ClientToScreen.argtypes = [wt.HWND, ctypes.c_void_p]
    u.ClientToScreen(hwnd, ctypes.byref(org))
    print('client origin', org.x, org.y, 'size', cr.right, cr.bottom)
    x = org.x + 300
    samples = {}
    y = org.y + 10
    y_end = org.y + 930
    while y < y_end:
        u.SetCursorPos(x, y)
        time.sleep(0.18)
        h = get_cursor()
        samples.setdefault(h, []).append(y)
        y += 8
    print('distinct handles:', len(samples))
    for h, ys in samples.items():
        print('  h=%-12s rows=%d y=%d..%d' % (h, len(ys), min(ys), max(ys)))
    sys_arrow = u.LoadCursorW(None, ctypes.c_void_p(32512))
    sys_hand = u.LoadCursorW(None, ctypes.c_void_p(32649))
    sys_ibeam = u.LoadCursorW(None, ctypes.c_void_p(32513))
    print('sys arrow=%s hand=%s ibeam=%s' % (sys_arrow, sys_hand, sys_ibeam))

    # 在“张开手”那一行按下左键，取 grabbing 光标
    grab_handle = None
    for h, ys in samples.items():
        if h and h not in (sys_arrow, sys_hand, sys_ibeam) and len(ys) <= 14 and len(ys) >= 8:
            grab_handle = h
            grab_y = sum(ys) // len(ys)
            break
    if grab_handle:
        u.SetCursorPos(x, grab_y)
        time.sleep(0.3)
        u.mouse_event(0x0002, 0, 0, 0, 0)
        time.sleep(0.35)
        h_down = get_cursor()
        u.mouse_event(0x0004, 0, 0, 0, 0)
        samples.setdefault(h_down, []).append(grab_y)
        print('pressed cursor h=%s (grabbed from y=%d)' % (h_down, grab_y))

    sheet_rows = []
    cell = 32 * 6
    for h, ys in samples.items():
        if not h:
            continue
        label = 'h%d' % h
        ok, rows = render(h, 32, 6)
        sheet_rows.append((label, rows))
        hs = dump_cur(h, os.path.join(OUT_DIR, '%s.cur' % label))
        save_png(os.path.join(OUT_DIR, '%s.png' % label), cell, cell, rows)
        print('%-12s hotspot=%s rows=%d' % (label, hs, len(ys)))

    W = cell * len(sheet_rows) + 8 * (len(sheet_rows) + 1)
    H = cell + 16
    sheet = [bytearray(b'\xff\xff\xff' * W) for _ in range(H)]
    x0 = 8
    for label, rows in sheet_rows:
        for y, row in enumerate(rows):
            sheet[y + 8][x0 * 3:(x0 + cell) * 3] = row
        x0 += cell + 8
    save_png(os.path.join(OUT_DIR, '_sheet.png'), W, H, sheet)
    u.SetCursorPos(rect.left + 300, rect.top + 200)
    proc.terminate()
    print('done ->', OUT_DIR)


if __name__ == '__main__':
    main()
