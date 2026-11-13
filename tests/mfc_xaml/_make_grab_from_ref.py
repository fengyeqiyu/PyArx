# -*- coding: utf-8 -*-
"""从浏览器 grab 光标截图（docs\\_grab_grid.txt 的 48x48 灰度）生成 CUR。

思路：保留截图里原有的黑色笔画（它已经把指缝画出来了），只把内部填白、
外部去掉 —— 这样形状和指缝都跟浏览器那只一模一样。
48 和 32 两个尺寸都直接从灰度网格缩放到目标尺寸再二值化，
所以 32px 下指缝依然是 1px 细线，不会糊成连指手套。
"""
import struct

GRID = r'G:\PyArx\docs\_grab_grid.txt'
OUT = r'G:\PyArx\res\grab.cur'
PREVIEW = r'G:\PyArx\docs\_grab_cursor_preview.png'
BIG = 48
SMALL = 32
STROKE_TH = 95
HOTSPOT = (20, 26)          # 48x48 坐标系下的掌心


def load_grid():
    with open(GRID, 'r', encoding='utf-8') as f:
        lines = [ln.strip() for ln in f if ln.strip()]
    return [[int(ln[i:i + 2], 16) for i in range(0, BIG * 2, 2)] for ln in lines[:BIG]]


def lum_at(grid, size):
    """把 48x48 灰度按面积平均缩放到 size×size。"""
    if size == BIG:
        return [[float(grid[y][x]) for x in range(BIG)] for y in range(BIG)]
    out = [[0.0] * size for _ in range(size)]
    k = BIG / size
    for y in range(size):
        for x in range(size):
            s = 0.0
            n = 0
            for sy in range(int(y * k), min(BIG, int((y + 1) * k))):
                for sx in range(int(x * k), min(BIG, int((x + 1) * k))):
                    s += grid[sy][sx]
                    n += 1
            out[y][x] = s / n if n else 255.0
    return out


def compose(lum, size):
    strokes = [[lum[y][x] < STROKE_TH for x in range(size)] for y in range(size)]
    # 从四周 flood fill 得到外部；剩下的算手形
    outside = [[False] * size for _ in range(size)]
    stack = []
    for x in range(size):
        for y in (0, size - 1):
            if not strokes[y][x]:
                outside[y][x] = True
                stack.append((x, y))
    for y in range(size):
        for x in (0, size - 1):
            if not strokes[y][x] and not outside[y][x]:
                outside[y][x] = True
                stack.append((x, y))
    while stack:
        x, y = stack.pop()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < size and 0 <= ny < size and not strokes[ny][nx] and not outside[ny][nx]:
                outside[ny][nx] = True
                stack.append((nx, ny))

    px = [[(0, 0, 0, 0)] * size for _ in range(size)]
    for y in range(size):
        for x in range(size):
            if outside[y][x]:
                px[y][x] = (0, 0, 0, 0)
            elif strokes[y][x]:
                px[y][x] = (0, 0, 0, 255)
            else:
                px[y][x] = (255, 255, 255, 255)
    return px


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


def main():
    grid = load_grid()
    sizes = (BIG, SMALL)
    blobs = {}
    for s in sizes:
        blobs[s] = image_bytes(compose(lum_at(grid, s), s), s)

    head = struct.pack('<HHH', 0, 2, len(sizes))
    hdr_len = 6 + 16 * len(sizes)
    entries = b''
    off = 0
    for s in sizes:
        hx = int(HOTSPOT[0] * s / BIG)
        hy = int(HOTSPOT[1] * s / BIG)
        entries += struct.pack('<BBBBHHII', s % 256, s % 256, 0, 0, hx, hy, len(blobs[s]), hdr_len + off)
        off += len(blobs[s])
    body = b''.join(blobs[s] for s in sizes)
    with open(OUT, 'wb') as f:
        f.write(head + entries + body)
    print('written', OUT, len(head + entries + body), 'bytes')

    import zlib

    def png_chunk(tag, data):
        return (struct.pack('>I', len(data)) + tag + data +
                struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF))

    def write_png(path, px, size, bg, scale):
        raw = bytearray()
        for y in range(size):
            for _ in range(scale):
                line = bytearray()
                for x in range(size):
                    r, g, b, a = px[y][x]
                    k = a / 255.0
                    line += bytes((int(r * k + bg * (1 - k)),
                                   int(g * k + bg * (1 - k)),
                                   int(b * k + bg * (1 - k))) * scale)
                raw += b'\x00' + line
        png = (b'\x89PNG\r\n\x1a\n' +
               png_chunk(b'IHDR', struct.pack('>IIBBBBB', size * scale, size * scale, 8, 2, 0, 0, 0)) +
               png_chunk(b'IDAT', zlib.compress(bytes(raw), 9)) +
               png_chunk(b'IEND', b''))
        with open(path, 'wb') as f:
            f.write(png)

    px32 = compose(lum_at(grid, SMALL), SMALL)
    write_png(PREVIEW, px32, SMALL, 255, 8)
    write_png(PREVIEW.replace('.png', '_gray.png'), px32, SMALL, 205, 8)
    print('preview written')


if __name__ == '__main__':
    main()
