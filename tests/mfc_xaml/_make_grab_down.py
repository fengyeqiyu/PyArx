# -*- coding: utf-8 -*-
"""生成"握住的"（grabbing）光标：握拳 —— 掌 + 四指关节凸起 + 拇指，1px 描边。
尺寸/比例与 grab.cur 保持一致（32×32，手形占画布 58%）。
"""
import math
import struct
import zlib

OUT = r'G:\PyArx\res\grab_down.cur'
PREVIEW = r'G:\PyArx\docs\_grab_down_preview.png'
SIZE = 32
FILL = 0.58
SS = 4


def shape_at(px, py):
    """归一化坐标（0..1）判断是否在拳形内。"""
    # 手掌：圆角矩形
    cx, cy, hw, hh, r = 0.5, 0.60, 0.235, 0.245, 0.10
    dx = abs(px - cx) - (hw - r)
    dy = abs(py - cy) - (hh - r)
    if dx <= 0 and dy <= 0:
        in_palm = True
    else:
        in_palm = (max(dx, 0.0) ** 2 + max(dy, 0.0) ** 2) <= r * r
    # 四个指关节凸起（上方）
    knuckle_y = 0.365
    for i in range(4):
        kx = 0.335 + i * 0.115
        kr = 0.062
        if ((px - kx) ** 2 + (py - knuckle_y) ** 2) <= kr * kr:
            return True
    # 拇指（左侧包过来）
    tx, ty, tr = 0.245, 0.555, 0.105
    if ((px - tx) ** 2 + (py - ty) ** 2) <= tr * tr:
        return True
    return in_palm


def build():
    px = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            hit = 0
            for i in range(SS):
                for j in range(SS):
                    nx = (x + (j + 0.5) / SS) / SIZE
                    ny = (y + (i + 0.5) / SS) / SIZE
                    # 先按 FILL 收缩到画布中央
                    nx = (nx - 0.5) / FILL + 0.5
                    ny = (ny - 0.5) / FILL + 0.5
                    if shape_at(nx, ny):
                        hit += 1
            row.append(hit / (SS * SS))
        px.append(row)

    # 覆盖率 → alpha；1px 内描边
    alpha = [[int(round(v * 255)) for v in row] for row in px]
    solid = [[px[y][x] >= 0.5 for x in range(SIZE)] for y in range(SIZE)]

    dist = [[10 ** 6] * SIZE for _ in range(SIZE)]
    q = []
    for y in range(SIZE):
        for x in range(SIZE):
            if solid[y][x]:
                edge = False
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if not (0 <= nx < SIZE and 0 <= ny < SIZE) or not solid[ny][nx]:
                        edge = True
                        break
                if edge:
                    dist[y][x] = 1
                    q.append((x, y))
    head = 0
    while head < len(q):
        x, y = q[head]
        head += 1
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < SIZE and 0 <= ny < SIZE and solid[ny][nx] and dist[ny][nx] > dist[y][x] + 1:
                dist[ny][nx] = dist[y][x] + 1
                q.append((nx, ny))

    out = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            a = alpha[y][x]
            if a == 0:
                row.append((0, 0, 0, 0))
            elif dist[y][x] <= 1:
                row.append((0, 0, 0, a))
            else:
                row.append((255, 255, 255, a))
        out.append(row)
    return out


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
    px = build()
    blob = image_bytes(px, SIZE)
    hx, hy = int(round(0.5 * SIZE)), int(round(0.58 * SIZE))
    head = struct.pack('<HHH', 0, 2, 1)
    entry = struct.pack('<BBBBHHII', SIZE % 256, SIZE % 256, 0, 0, hx, hy, len(blob), 6 + 16)
    with open(OUT, 'wb') as f:
        f.write(head + entry + blob)
    print('written', OUT, 6 + 16 + len(blob), 'bytes hotspot', (hx, hy))

    def ck(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)

    scale = 8
    raw = bytearray()
    for y in range(SIZE):
        for _ in range(scale):
            line = bytearray()
            for x in range(SIZE):
                r, g, b, a = px[y][x]
                k = a / 255.0
                line += bytes((int(r * k + 255 * (1 - k)),
                               int(g * k + 255 * (1 - k)),
                               int(b * k + 255 * (1 - k))) * scale)
            raw += b'\x00' + line
    with open(PREVIEW, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' +
                ck(b'IHDR', struct.pack('>IIBBBBB', SIZE * scale, SIZE * scale, 8, 2, 0, 0, 0)) +
                ck(b'IDAT', zlib.compress(bytes(raw), 9)) + ck(b'IEND', b''))
    print('preview written')


if __name__ == '__main__':
    main()
