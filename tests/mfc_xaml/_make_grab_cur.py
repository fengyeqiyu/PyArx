# -*- coding: utf-8 -*-
"""从浏览器 grab 光标截图（docs\\_grab_grid.txt 的 48x48 灰度）生成 CUR。

1) 二值求手形（描边+填充的并集），flood fill 去掉外部；
2) 在 48x48 上按"到手形边缘的距离"重画 1px 细描边（源图描边被放大得偏粗）；
3) 指缝：沿顶部轮廓的凹口往下补几像素竖线，把四指分开；
4) 32x32 由 48x48 面积平均降采样（预乘 alpha），线条自然抗锯齿。
热区在掌心。
"""
import struct

GRID = r'G:\PyArx\docs\_grab_grid.txt'
OUT = r'G:\PyArx\res\grab.cur'
PREVIEW = r'G:\PyArx\docs\_grab_cursor_preview.png'
BIG = 48
SMALL = 32
STROKE_TH = 128
HOTSPOT = (20, 26)      # 旧口径，已改为按包围盒换算
FILL = 0.58             # 手形占画布的比例（留边，避免图标顶满显得过大）
SEP_LEN = 7             # 指缝竖线长度（48 坐标）
LOW, HIGH = 115, 205   # 对比度拉伸的低/高阈值


def load_grid():
    with open(GRID, 'r', encoding='utf-8') as f:
        lines = [ln.strip() for ln in f if ln.strip()]
    return [[int(ln[i:i + 2], 16) for i in range(0, BIG * 2, 2)] for ln in lines[:BIG]]


def build(grid):
    dark = [[grid[y][x] < STROKE_TH for x in range(BIG)] for y in range(BIG)]
    outside = [[False] * BIG for _ in range(BIG)]
    stack = []
    for x in range(BIG):
        for y in (0, BIG - 1):
            if not dark[y][x]:
                outside[y][x] = True
                stack.append((x, y))
    for y in range(BIG):
        for x in (0, BIG - 1):
            if not dark[y][x] and not outside[y][x]:
                outside[y][x] = True
                stack.append((x, y))
    while stack:
        x, y = stack.pop()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < BIG and 0 <= ny < BIG and not dark[ny][nx] and not outside[ny][nx]:
                outside[ny][nx] = True
                stack.append((nx, ny))
    shape = [[not outside[y][x] for x in range(BIG)] for y in range(BIG)]

    dist = [[10 ** 6] * BIG for _ in range(BIG)]
    q = []
    for y in range(BIG):
        for x in range(BIG):
            if shape[y][x]:
                edge = False
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if not (0 <= nx < BIG and 0 <= ny < BIG) or not shape[ny][nx]:
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
            if 0 <= nx < BIG and 0 <= ny < BIG and shape[ny][nx] and dist[ny][nx] > dist[y][x] + 1:
                dist[ny][nx] = dist[y][x] + 1
                q.append((nx, ny))

    px = [[(0, 0, 0, 0)] * BIG for _ in range(BIG)]
    for y in range(BIG):
        for x in range(BIG):
            if not shape[y][x]:
                continue
            v = grid[y][x]
            # 截图本身是放大过的，笔画和填充的灰度都偏灰；做一次对比度拉伸，
            # 让描边变回纯黑、掌心变回纯白（形状不动）。
            v = 0 if v <= LOW else (255 if v >= HIGH else int((v - LOW) * 255.0 / (HIGH - LOW)))
            px[y][x] = (v, v, v, 255)

    prof = [None] * BIG
    for x in range(BIG):
        for y in range(BIG):
            if shape[y][x]:
                prof[x] = y
                break
    valleys = []
    for x in range(1, BIG - 1):
        if prof[x] is None:
            continue
        l = prof[x - 1] if prof[x - 1] is not None else prof[x]
        r = prof[x + 1] if prof[x + 1] is not None else prof[x]
        if prof[x] > l and prof[x] >= r:
            valleys.append(x)
    keep = []
    for x in valleys:
        if keep and x - keep[-1] <= 2:
            continue
        keep.append(x)
    for x in []:
        y0 = prof[x]
        for y in range(y0, min(BIG, y0 + SEP_LEN)):
            if shape[y][x]:
                px[y][x] = (0, 0, 0, 255)
    return px


def downsample(px, size):
    """把 48x48 的原图整体（含留白）重排到 size×size 画布：
    手形只占画布的 FILL 比例，四周留透明边——和浏览器光标一样，指针不能顶满画布。"""
    # 先求手形包围盒
    xs = [x for y in range(BIG) for x in range(BIG) if px[y][x][3]]
    ys = [y for y in range(BIG) for x in range(BIG) if px[y][x][3]]
    if not xs:
        raise SystemExit('empty shape')
    bx0, bx1 = min(xs), max(xs)
    by0, by1 = min(ys), max(ys)
    bw = bx1 - bx0 + 1
    bh = by1 - by0 + 1

    target_h = size * FILL
    k = target_h / bh
    target_w = bw * k
    ox = (size - target_w) / 2.0
    oy = (size - target_h) / 2.0

    out = [[(0, 0, 0, 0)] * size for _ in range(size)]
    ss = 3
    for y in range(size):
        for x in range(size):
            r = g = b = a = 0
            for i in range(ss):
                for j in range(ss):
                    dx = x + (j + 0.5) / ss
                    dy = y + (i + 0.5) / ss
                    sx = (dx - ox) / k + bx0
                    sy = (dy - oy) / k + by0
                    ix, iy = int(sx), int(sy)
                    if ix < 0 or iy < 0 or ix >= BIG or iy >= BIG:
                        continue
                    sr, sg, sb, sa = px[iy][ix]
                    r += sr * sa
                    g += sg * sa
                    b += sb * sa
                    a += sa
            n = ss * ss
            if a == 0:
                out[y][x] = (0, 0, 0, 0)
            else:
                out[y][x] = (r // a, g // a, b // a, a // n)
    return out, (ox + target_w * 0.42, oy + target_h * 0.58)


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


def thin_strokes(px, size):
    """Zhang-Suen 细化：把 2~3 像素粗的黑描边压到 1 像素，alpha 不动。"""
    mask = [[px[y][x][3] > 0 and px[y][x][0] < 100 for x in range(size)] for y in range(size)]

    def neighbours(y, x):
        return [mask[y - 1][x], mask[y - 1][x + 1], mask[y][x + 1], mask[y + 1][x + 1],
                mask[y + 1][x], mask[y + 1][x - 1], mask[y][x - 1], mask[y - 1][x - 1]]

    def trans(p):
        return sum(1 for i in range(8) if (not p[i]) and p[(i + 1) % 8])

    for _ in range(24):
        changed = False
        for phase in (0, 1):
            kill = []
            for y in range(1, size - 1):
                for x in range(1, size - 1):
                    if not mask[y][x]:
                        continue
                    p = neighbours(y, x)
                    s = sum(1 for v in p if v)
                    if s < 2 or s > 6 or trans(p) != 1:
                        continue
                    if phase == 0:
                        if (p[0] and p[2] and p[4]) or (p[2] and p[4] and p[6]):
                            continue
                    else:
                        if (p[0] and p[2] and p[6]) or (p[0] and p[4] and p[6]):
                            continue
                    kill.append((y, x))
            for y, x in kill:
                mask[y][x] = False
            if kill:
                changed = True
        if not changed:
            break

    out = [[(0, 0, 0, 0)] * size for _ in range(size)]
    for y in range(size):
        for x in range(size):
            a = px[y][x][3]
            out[y][x] = (0, 0, 0, a) if mask[y][x] else (255, 255, 255, a)
    return out


def main():
    master = build(load_grid())
    sizes = (SMALL,)
    blobs = {}
    hots = {}
    for s in sizes:
        px, hs = downsample(master, s)
        # 颜色二值化（笔画纯黑、填充纯白），alpha 保留重采样的软边，线条干净不糊
        for y in range(s):
            for x in range(s):
                r, g, b, a = px[y][x]
                v = 0 if (r + g + b) / 3.0 < 165 else 255
                px[y][x] = (v, v, v, a)
        px = thin_strokes(px, s)
        blobs[s] = image_bytes(px, s)
        hots[s] = (int(round(hs[0])), int(round(hs[1])))
        if s == SMALL:
            px32 = px
        if s == BIG:
            px48 = px

    head = struct.pack('<HHH', 0, 2, len(sizes))
    hdr_len = 6 + 16 * len(sizes)
    entries = b''
    off = 0
    for s in sizes:
        entries += struct.pack('<BBBBHHII', s % 256, s % 256, 0, 0,
                               hots[s][0], hots[s][1],
                               len(blobs[s]), hdr_len + off)
        print('size %d hotspot %s' % (s, hots[s]))
        off += len(blobs[s])
    with open(OUT, 'wb') as f:
        f.write(head + entries + b''.join(blobs[s] for s in sizes))
    print('written', OUT, hdr_len + sum(len(blobs[s]) for s in sizes), 'bytes')

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

    write_png(PREVIEW, px32, SMALL, 255, 8)
    write_png(PREVIEW.replace('.png', '_gray.png'), px32, SMALL, 205, 8)
    print('preview written')


if __name__ == '__main__':
    main()
