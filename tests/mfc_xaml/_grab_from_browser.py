# -*- coding: utf-8 -*-
"""把从浏览器 HCURSOR 里抓出来的真光标（张开手 / 握拳）按项目认可的尺寸重排成 .cur。
源文件是 _grab_browser_cursor.py 导出的 32x32 32bpp 光标，解码后按手形高度缩放，
保持浏览器原始造型与 1px 描边，热区同比缩放。"""
import ctypes
import os
import struct
import zlib
from ctypes import wintypes as wt

SRC_DIR = r'G:\PyArx\docs\_browser_cursors'
SRC_GRAB = os.path.join(SRC_DIR, 'h973018771.cur')   # 张开手 cursor:grab
SRC_FIST = os.path.join(SRC_DIR, 'h711336981.cur')   # 握拳  cursor:grabbing
OUT_DIR = r'G:\PyArx\res'
PREVIEW = r'G:\PyArx\docs\_grab_browser_cmp.png'
SIZE = 32
FILL = 0.58          # 与已认可版本一致：手形高度占 32px 的 58%
HOTSPOT_SRC = (13, 13)


def png(path, w, h, rows):
    raw = b''.join(b'\x00' + bytes(r) for r in rows)
    ck = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)
    data = (b'\x89PNG\r\n\x1a\n' + ck(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) +
            ck(b'IDAT', zlib.compress(raw, 9)) + ck(b'IEND', b''))
    with open(path, 'wb') as f:
        f.write(data)


def read_cur(path):
    raw = open(path, 'rb').read()
    off = struct.unpack('<I', raw[18:22])[0]
    bi = struct.unpack('<IiiHHI', raw[off:off + 20])
    width, height, bits = bi[1], bi[2] // 2, bi[4]
    xor = raw[off + 40:off + 40 + width * height * 4]
    rowbytes = ((width + 31) // 32) * 4
    andmask = raw[off + 40 + len(xor):off + 40 + len(xor) + rowbytes * height]
    px = [[(0, 0, 0, 0)] * width for _ in range(height)]
    for y in range(height):
        sy = height - 1 - y
        for x in range(width):
            i = (sy * width + x) * 4
            b, g, r, a = xor[i], xor[i + 1], xor[i + 2], xor[i + 3]
            if a == 0:
                bit = (andmask[sy * rowbytes + x // 8] >> (7 - x % 8)) & 1
                a = 0 if bit else 255
            px[y][x] = (r, g, b, a)
    return px, width, height


def bbox(px, w, h):
    xs = [x for y in range(h) for x in range(w) if px[y][x][3] > 0]
    ys = [y for y in range(h) for x in range(w) if px[y][x][3] > 0]
    return min(xs), min(ys), max(xs), max(ys)


def resample(px, w, h, size, fill, hotspot):
    bx0, by0, bx1, by1 = bbox(px, w, h)
    bw, bh = bx1 - bx0 + 1, by1 - by0 + 1
    th = size * fill
    k = th / bh
    tw = bw * k
    ox = (size - tw) / 2.0
    oy = (size - th) / 2.0
    out = [[(0, 0, 0, 0)] * size for _ in range(size)]
    ss = 3
    for y in range(size):
        for x in range(size):
            r = g = b = a = 0
            for j in range(ss):
                for i in range(ss):
                    sx = (x + (j + 0.5) / ss - ox) / k + bx0
                    sy = (y + (i + 0.5) / ss - oy) / k + by0
                    ix, iy = int(sx), int(sy)
                    if not (0 <= ix < w and 0 <= iy < h):
                        continue
                    sr, sg, sb, sa = px[iy][ix]
                    aw = sa / 255.0
                    r += sr * aw
                    g += sg * aw
                    b += sb * aw
                    a += sa
            n = ss * ss
            if a == 0:
                out[y][x] = (0, 0, 0, 0)
            else:
                out[y][x] = (round(r / (a / 255.0)), round(g / (a / 255.0)), round(b / (a / 255.0)),
                             a // n)
    hx = int(round(ox + (hotspot[0] - bx0) * k))
    hy = int(round(oy + (hotspot[1] - by0) * k))
    hx = max(0, min(size - 1, hx))
    hy = max(0, min(size - 1, hy))
    return out, (hx, hy)


def image_bytes(px, size):
    xor = bytearray()
    for y in range(size - 1, -1, -1):
        for x in range(size):
            r, g, b, a = px[y][x]
            xor += bytes((b, g, r, a))
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


def write_cur(px, path, hotspot):
    body = image_bytes(px, SIZE)
    hdr = struct.pack('<HHH', 0, 2, 1)
    hdr += struct.pack('<BBBBHHII', SIZE, SIZE, 0, 0, hotspot[0], hotspot[1],
                       len(body), 22)
    with open(path, 'wb') as f:
        f.write(hdr + body)
    return len(hdr) + len(body)


def render_png(px, scale, path):
    rows = []
    for y in range(SIZE * scale):
        sy = y // scale
        row = bytearray()
        for x in range(SIZE * scale):
            sx = x // scale
            r, g, b, a = px[sy][sx]
            if a == 0:
                row += bytes((0xE6, 0xE6, 0xE6))
            else:
                w_ = a / 255.0
                row += bytes((int(r * w_ + 0xE6 * (1 - w_)), int(g * w_ + 0xE6 * (1 - w_)),
                              int(b * w_ + 0xE6 * (1 - w_))))
        rows.append(row)
    png(path, SIZE * scale, SIZE * scale, rows)


def verify(path):
    u = ctypes.windll.user32
    u.LoadImageW.restype = wt.HANDLE
    u.LoadImageW.argtypes = [wt.HINSTANCE, wt.LPCWSTR, wt.UINT, ctypes.c_int, ctypes.c_int, wt.UINT]
    h = u.LoadImageW(None, path, 2, 0, 0, 0x10)
    return bool(h)


def main():
    out = []
    for src, name in ((SRC_GRAB, 'grab.cur'), (SRC_FIST, 'grab_down.cur')):
        px, w, h = read_cur(src)
        small, hot = resample(px, w, h, SIZE, FILL, HOTSPOT_SRC)
        dst = os.path.join(OUT_DIR, name)
        size_bytes = write_cur(small, dst, hot)
        ok = verify(dst)
        print('%s -> %s  bytes=%d hotspot=%s LoadImage=%s' % (os.path.basename(src), dst, size_bytes, hot, ok))
        out.append(small)
    cell = SIZE * 8
    W = cell * len(out) + 10 * (len(out) + 1)
    H = cell + 20
    sheet = [bytearray(b'\xff\xff\xff' * W) for _ in range(H)]
    for i, px in enumerate(out):
        rows = []
        for y in range(cell):
            sy = y // 8
            row = bytearray()
            for x in range(cell):
                sx = x // 8
                r, g, b, a = px[sy][sx]
                if a == 0:
                    row += bytes((230, 230, 230))
                else:
                    w_ = a / 255.0
                    row += bytes((int(r * w_ + 230 * (1 - w_)), int(g * w_ + 230 * (1 - w_)),
                                  int(b * w_ + 230 * (1 - w_))))
            rows.append(row)
        x0 = 10 + i * (cell + 10)
        for y, row in enumerate(rows):
            sheet[y + 10][x0 * 3:(x0 + cell) * 3] = row
    png(PREVIEW, W, H, sheet)
    print('preview ->', PREVIEW)


if __name__ == '__main__':
    main()
