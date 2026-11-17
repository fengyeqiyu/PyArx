"""截取 PyArx Viewer 窗口（PrintWindow + GetDIBits，不依赖 PIL/System.Drawing）。

用法：python shot.py <输出.png> [标题]
"""
import ctypes
import ctypes.wintypes as wt
import os
import struct
import sys
import time
import zlib

u32 = ctypes.windll.user32
g32 = ctypes.windll.gdi32


class BIH(ctypes.Structure):
    _fields_ = [("biSize", wt.DWORD), ("biWidth", wt.LONG), ("biHeight", wt.LONG),
                ("biPlanes", wt.WORD), ("biBitCount", wt.WORD), ("biCompression", wt.DWORD),
                ("biSizeImage", wt.DWORD), ("biXPelsPerMeter", wt.LONG),
                ("biYPelsPerMeter", wt.LONG), ("biClrUsed", wt.DWORD),
                ("biClrImportant", wt.DWORD)]


def write_png(path, w, h, bgra):
    raw = bytearray()
    for y in range(h):
        line = bgra[y * w * 4:(y + 1) * w * 4]
        raw.append(0)
        raw += b"".join(line[i:i + 3][::-1] for i in range(0, w * 4, 4))

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(bytes(raw), 6))
    out += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(out)


def grab(out_path, title="PyArx Viewer", from_screen=False):
    hwnd = u32.FindWindowW(None, title)
    if not hwnd:
        print("找不到窗口:", title)
        return 1
    u32.ShowWindow(hwnd, 9)                 # SW_RESTORE
    u32.SetForegroundWindow(hwnd)
    time.sleep(0.6)
    r = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    if w <= 0 or h <= 0:
        print("窗口尺寸异常", w, h)
        return 1

    if from_screen:
        src = u32.GetDC(0)                  # 屏幕 DC：含 DWM 合成的标题栏与 WebView2
        srcy = r.top
    else:
        src = u32.GetWindowDC(hwnd)
        srcy = 0

    mem = g32.CreateCompatibleDC(src)
    bmp = g32.CreateCompatibleBitmap(src, w, h)
    g32.SelectObject(mem, bmp)
    if from_screen:
        g32.BitBlt(mem, 0, 0, w, h, src, r.left, srcy, 0x00CC0020)   # SRCCOPY
    else:
        u32.PrintWindow(hwnd, mem, 2)          # PW_RENDERFULLCONTENT

    bi = BIH()
    bi.biSize = ctypes.sizeof(bi)
    bi.biWidth = w
    bi.biHeight = -h
    bi.biPlanes = 1
    bi.biBitCount = 32
    buf = ctypes.create_string_buffer(w * h * 4)
    g32.GetDIBits(mem, bmp, 0, h, buf, ctypes.byref(bi), 0)

    g32.DeleteObject(bmp)
    g32.DeleteDC(mem)
    if from_screen:
        u32.ReleaseDC(0, src)
    else:
        u32.ReleaseDC(hwnd, src)

    write_png(out_path, w, h, buf.raw)
    print("截图 %s  %dx%d  %.0f KB" % (out_path, w, h, os.path.getsize(out_path) / 1024.0))
    return 0


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a != "--screen"]
    use_screen = "--screen" in sys.argv
    out = args[0] if args else "shot.png"
    title = args[1] if len(args) > 1 else "PyArx Viewer"
    sys.exit(grab(out, title, use_screen))
