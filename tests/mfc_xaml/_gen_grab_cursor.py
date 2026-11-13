# -*- coding: utf-8 -*-
"""生成 res\grab.cur：32x32 张开的手（爪型抓取光标）。

Windows 光标表里没有 grab 编号（只有 IDC_HAND 指向手），按浏览器 cursor:grab
的观感自己画：四指 + 掌心 + 左伸拇指，用一条闭合轮廓（不是几个胶囊拼），
指缝靠轮廓上的 V 形凹口分出来，白填充 + 1.1px 深色描边 + 右下软投影。
热区在掌心 (16,16)。
"""
import math
import struct

W = H = 32
SS = 6
OUTLINE = 1.15
ROUND = 0.45      # 轮廓外扩半径：把尖锐的指尖/凹口磨圆
DARK = (20, 20, 20)
LIGHT = (255, 255, 255)

# 闭合轮廓（顺时针）：掌心底 → 左掌 → 拇指 → 食指 → 中指 → 无名指 → 小指 → 右侧 → 底
OUTLINE_PTS = [
    (8.6, 22.6),
    (5.0, 20.4),
    (3.6, 19.0),    # 拇指尖
    (5.6, 16.8),
    (8.8, 16.4),    # 拇指根部 / 食指根
    (9.2, 9.6),
    (10.4, 7.6),    # 食指尖
    (11.4, 8.6),
    (12.0, 14.2),   # 食指/中指凹口
    (12.9, 11.4),
    (14.4, 4.4),    # 中指尖
    (15.6, 11.4),
    (16.4, 14.4),   # 中指/无名凹口
    (17.0, 10.4),
    (18.6, 5.6),    # 无名指尖
    (19.8, 10.4),
    (20.6, 14.6),   # 无名/小指凹口
    (21.2, 11.6),
    (22.6, 9.6),    # 小指尖
    (23.8, 12.2),
    (24.4, 16.0),
    (24.6, 21.6),
    (23.0, 25.4),
    (19.0, 27.4),
    (13.2, 27.6),
    (9.4, 26.8),
]


def dist_seg(px, py, ax, ay, bx, by):
    vx, vy = bx - ax, by - ay
    wx, wy = px - ax, py - ay
    L2 = vx * vx + vy * vy
    t = 0.0 if L2 == 0 else max(0.0, min(1.0, (wx * vx + wy * vy) / L2))
    return math.hypot(px - (ax + vx * t), py - (ay + vy * t))


def sdf(x, y):
    """< 0 表示在轮廓内；绝对值 = 到轮廓的距离（近似，够画图用）。"""
    n = len(OUTLINE_PTS)
    d = 1e9
    inside = False
    for i in range(n):
        ax, ay = OUTLINE_PTS[i]
        bx, by = OUTLINE_PTS[(i + 1) % n]
        d = min(d, dist_seg(x, y, ax, ay, bx, by))
        if (ay > y) != (by > y):
            xi = ax + (y - ay) * (bx - ax) / (by - ay)
            if x < xi:
                inside = not inside
    return (-d if inside else d) - ROUND


def main():
    px_rows = []
    for y in range(H):
        row = []
        for x in range(W):
            inside = ring = sh = n = 0
            for sy in range(SS):
                for sx in range(SS):
                    fx = x + (sx + 0.5) / SS
                    fy = y + (sy + 0.5) / SS
                    d = sdf(fx, fy)
                    n += 1
                    if d <= 0.0:
                        inside += 1
                    if d <= OUTLINE:
                        ring += 1
                    if sdf(fx + 1.1, fy + 1.1) <= 0.0:
                        sh += 1
            cover = ring / n
            shadow = 0.28 * sh / n
            if cover <= 0.0:
                row.append((0, 0, 0, int(round(255 * shadow))))
                continue
            if inside / n > 0.55:
                row.append((LIGHT[0], LIGHT[1], LIGHT[2], int(round(255 * max(inside / n, cover)))))
            else:
                row.append((DARK[0], DARK[1], DARK[2], int(round(255 * cover))))
        px_rows.append(row)

    xor = bytearray()
    for y in range(H - 1, -1, -1):
        for x in range(W):
            r, g, b, a = px_rows[y][x]
            xor += bytes((b, g, r, a))

    rowbytes = ((W + 31) // 32) * 4
    andmask = bytearray()
    for y in range(H - 1, -1, -1):
        row = bytearray(rowbytes)
        for x in range(W):
            if px_rows[y][x][3] == 0:
                row[x // 8] |= 0x80 >> (x % 8)
        andmask += row

    bi = struct.pack('<IiiHHI', 40, W, H * 2, 1, 32, 0)
    bi += struct.pack('<IiiII', len(xor) + len(andmask), 0, 0, 0, 0)
    image = bi + bytes(xor) + bytes(andmask)
    hdr = struct.pack('<HHH', 0, 2, 1)
    entry = struct.pack('<BBBBHHII', W % 256, H % 256, 0, 0, 16, 16, len(image), 22)
    out = r'G:\PyArx\res\grab.cur'
    with open(out, 'wb') as f:
        f.write(hdr + entry + image)
    print('written', out, len(hdr + entry + image), 'bytes')

    import zlib

    def png_chunk(tag, data):
        return (struct.pack('>I', len(data)) + tag + data +
                struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF))

    def write_png(path, bg, scale=6):
        raw = bytearray()
        for y in range(H):
            for _ in range(scale):
                line = bytearray()
                for x in range(W):
                    r, g, b, a = px_rows[y][x]
                    k = a / 255.0
                    line += bytes((int(r * k + bg * (1 - k)),
                                   int(g * k + bg * (1 - k)),
                                   int(b * k + bg * (1 - k))) * scale)
                raw += b'\x00' + line
        png = (b'\x89PNG\r\n\x1a\n' +
               png_chunk(b'IHDR', struct.pack('>IIBBBBB', W * scale, H * scale, 8, 2, 0, 0, 0)) +
               png_chunk(b'IDAT', zlib.compress(bytes(raw), 9)) +
               png_chunk(b'IEND', b''))
        with open(path, 'wb') as f:
            f.write(png)

    write_png(r'G:\PyArx\docs\_grab_cursor_preview.png', 255)
    write_png(r'G:\PyArx\docs\_grab_cursor_preview_gray.png', 205)
    print('preview written')


if __name__ == '__main__':
    main()
