import struct, zlib, math, sys, os

# ---------------- SHX 容器 ----------------
class ShxFont:
    def __init__(self, path):
        d = open(path, 'rb').read()
        self.data = d
        self.signature = d[:32].split(b'\r')[0].decode('latin1')
        kind = self.signature
        if 'unifont' in kind:
            self.kind = 'unifont'; ofs = 0x19
        elif 'bigfont' in kind:
            self.kind = 'bigfont'; ofs = 0x1B
        else:
            self.kind = 'shapes'; ofs = 0x1C
        self.num = struct.unpack('<H', d[ofs:ofs+2])[0]
        self.glyphs = {}
        self.fontparams = None
        if self.kind == 'unifont':
            p = ofs + 2
            n = 0
            while p + 4 <= len(d) and n < self.num:
                code, ln = struct.unpack('<HH', d[p:p+4])
                if ln <= 0 or p + 4 + ln > len(d):
                    break
                self.glyphs[code] = d[p+4:p+4+ln]
                p += 4 + ln
                n += 1
            self.trailing = len(d) - p
        elif self.kind == 'bigfont':
            # 0x1B = 字形数, 0x1D = 变更块数, 变更块 4 字节/项, 之后 8 字节/项索引表(code u16,len u16,offset u32)
            count = struct.unpack('<H', d[0x1B:0x1D])[0]
            nchange = struct.unpack('<H', d[0x1D:0x1F])[0]
            T = 0x1F + 4 * nchange
            end = 0
            self.entries = 0
            for i in range(count):
                b = T + i * 8
                if b + 8 > len(d): break
                code, ln, off = struct.unpack('<HHI', d[b:b+8])
                self.entries += 1
                if off and ln and off + ln <= len(d):
                    self.glyphs[code] = d[off:off+ln]
                    end = max(end, off + ln)
            self.trailing = len(d) - end
        else:
            self.trailing = len(d)

def split_name(rec):
    """记录开头是名称串（0x00 结尾），字体字形通常为空名"""
    i = 0
    while i < len(rec) and rec[i] != 0:
        i += 1
    return rec[i+1:], rec[:i].decode('latin1')

# ---------------- 指令解码 ----------------
D45 = math.tan(math.radians(22.5))   # 0.4142

def dxy(direction, L):
    """16 方向 + 长度 -> (dx, dy)；对角方向长度取 |dx| 或 |dy| 中较大者"""
    if direction == 0:  return (L, 0)
    if direction == 4:  return (0, L)
    if direction == 8:  return (-L, 0)
    if direction == 12: return (0, -L)
    if direction == 2:  return (L, L)
    if direction == 6:  return (-L, L)
    if direction == 10: return (-L, -L)
    if direction == 14: return (L, -L)
    k = L * D45
    if direction == 1:  return (L, k)
    if direction == 3:  return (k, L)
    if direction == 5:  return (-k, L)
    if direction == 7:  return (-L, k)
    if direction == 9:  return (-L, -k)
    if direction == 11: return (-k, -L)
    if direction == 13: return (k, -L)
    if direction == 15: return (L, -k)
    return (0, 0)

def arc_points(cx, cy, r, a0, a1):
    """按 3° 步长离散圆弧（仅用于验证渲染）"""
    if a1 < a0:
        a0, a1 = a1, a0
    n = max(2, int((a1 - a0) / 3))
    return [(cx + r*math.cos(math.radians(a0 + (a1-a0)*i/n)),
             cy + r*math.sin(math.radians(a0 + (a1-a0)*i/n))) for i in range(n+1)]

def decode(rec, font=None, depth=0, stats=None):
    """返回 [(pts, closed_flag)]，坐标是 shx 单位（1 单位 ≈ 字体网格）"""
    code, name = split_name(rec)
    segs = []
    cur = [(0.0, 0.0)]
    pen = False
    scale = 1.0
    stack = []
    i = 0
    pos = (0.0, 0.0)

    def emit(p0, p1):
        if pen:
            segs.append(([p0, p1], False))

    while i < len(code):
        b = code[i]
        if b == 0:
            break
        L, d = b >> 4, b & 0xF
        if L == 0:
            op = d
            if stats is not None:
                stats[op] = stats.get(op, 0) + 1
            if op == 1:
                pen = True; i += 1
            elif op == 2:
                pen = False; i += 1
            elif op == 3:
                scale /= float(code[i+1]); i += 2
            elif op == 4:
                scale *= float(code[i+1]); i += 2
            elif op == 5:
                stack.append(pos); i += 1
            elif op == 6:
                if stack: pos = stack.pop()
                cur = [pos]; i += 1
            elif op == 7:
                sub = code[i+1]; i += 2
                if font and sub in font.glyphs and depth < 4:
                    for pts, cl in decode(font.glyphs[sub], font, depth+1)['segs']:
                        segs.append(([(pos[0]+x, pos[1]+y) for x, y in pts], cl))
            elif op == 8:
                dx = struct.unpack('b', bytes([code[i+1]]))[0]
                dy = struct.unpack('b', bytes([code[i+2]]))[0]
                i += 3
                nx, ny = pos[0] + dx*scale, pos[1] + dy*scale
                emit(pos, (nx, ny)); pos = (nx, ny); cur = [pos]
            elif op == 9:
                i += 1
                while i + 1 < len(code):
                    dx = struct.unpack('b', bytes([code[i]]))[0]
                    dy = struct.unpack('b', bytes([code[i+1]]))[0]
                    i += 2
                    if dx == 0 and dy == 0:
                        break
                    nx, ny = pos[0] + dx*scale, pos[1] + dy*scale
                    emit(pos, (nx, ny)); pos = (nx, ny)
                cur = [pos]
            elif op == 10:   # 八分弧：b1=半径, b2=sc(高4位=起始八分圆, 低4位=跨度数, bit7=顺时针)
                r = code[i+1] * scale
                sc = code[i+2]
                i += 3
                s = (sc >> 4) & 0x7
                c = sc & 0x7
                if c == 0: c = 8
                oct45 = math.pi / 4.0
                a0 = s * oct45
                sweep = c * oct45 * (-1.0 if (sc & 0x80) else 1.0)
                cx, cy = pos[0] - r * math.cos(a0), pos[1] - r * math.sin(a0)
                pts = [(cx + r*math.cos(a0 + sweep*k/len(range(25))),
                        cy + r*math.sin(a0 + sweep*k/len(range(25)))) for k in range(25)]
                if pen:
                    segs.append((pts, False))
                pos = pts[-1]
                cur = [pos]
            elif op == 11:   # 分数弧：start_off, end_off, r_hi, r_lo, sc
                so = code[i+1]
                eo = code[i+2]
                r = (256 * code[i+3] + code[i+4]) * scale
                sc = code[i+5]
                i += 6
                s = (sc >> 4) & 0x7
                c = sc & 0x7
                if c == 0: c = 8
                oct45 = math.pi / 4.0
                a0 = s * oct45 + oct45 * so / 256.0
                a1 = s * oct45 + c * oct45 + oct45 * eo / 256.0
                if sc & 0x80:
                    a0, a1 = a1, a0
                cx, cy = pos[0] - r * math.cos(a0), pos[1] - r * math.sin(a0)
                pts = [(cx + r*math.cos(a0 + (a1-a0)*k/24.0),
                        cy + r*math.sin(a0 + (a1-a0)*k/24.0)) for k in range(25)]
                if pen:
                    segs.append((pts, False))
                pos = pts[-1]
                cur = [pos]
            elif op == 12:   # 凸度弧
                dx = struct.unpack('b', bytes([code[i+1]]))[0]
                dy = struct.unpack('b', bytes([code[i+2]]))[0]
                bul = struct.unpack('b', bytes([code[i+3]]))[0] / 127.0
                i += 4
                p1 = (pos[0] + dx*scale, pos[1] + dy*scale)
                if pen:
                    segs.append(([pos, p1, bul], True))
                pos = p1
                cur = [pos]
            elif op == 13:   # 多凸度弧
                i += 1
                while i + 3 < len(code):
                    dx = struct.unpack('b', bytes([code[i]]))[0]
                    dy = struct.unpack('b', bytes([code[i+1]]))[0]
                    bul = struct.unpack('b', bytes([code[i+2]]))[0] / 127.0
                    i += 3
                    if dx == 0 and dy == 0:
                        break
                    p1 = (pos[0] + dx*scale, pos[1] + dy*scale)
                    if pen:
                        segs.append(([pos, p1, bul], True))
                    pos = p1
                cur = [pos]
            elif op == 14:
                i += 1
            else:
                i += 1
        else:
            nx = pos[0] + dxy(d, L)[0]*scale
            ny = pos[1] + dxy(d, L)[1]*scale
            emit(pos, (nx, ny))
            pos = (nx, ny); cur = [pos]
            i += 1
    return {'segs': segs, 'end': pos}

# ---------------- 渲染 ----------------
class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.px = bytearray(b'\xff' * (w*h))
    def line(self, x0, y0, x1, y1):
        x0, y0, x1, y1 = int(round(x0)), int(round(y0)), int(round(x1)), int(round(y1))
        dx, dy = abs(x1-x0), abs(y1-y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        while True:
            for ox in range(2):
                for oy in range(2):
                    if 0 <= x0+ox < self.w and 0 <= y0+oy < self.h:
                        self.px[(y0+oy)*self.w + x0+ox] = 0
            if x0 == x1 and y0 == y1: break
            e2 = 2*err
            if e2 > -dy: err -= dy; x0 += sx
            if e2 < dx: err += dx; y0 += sy
    def png(self, path):
        raw = b''.join(b'\x00' + bytes(self.px[y*self.w:(y+1)*self.w]) for y in range(self.h))
        def chunk(t, d):
            c = struct.pack('>I', len(d)) + t + d
            return c + struct.pack('>I', zlib.crc32(t+d) & 0xffffffff)
        hdr = struct.pack('>IIBBBBB', self.w, self.h, 8, 0, 0, 0, 0)
        png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', hdr) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
        open(path, 'wb').write(png)

def bulge_points(p0, p1, bulge):
    """凸度弧 -> 折线；bulge = tan(theta/4)"""
    if abs(bulge) < 1e-9:
        return [p0, p1]
    x0, y0 = p0; x1, y1 = p1
    c = math.hypot(x1 - x0, y1 - y0)
    theta = 4.0 * math.atan(bulge)
    r = c / (2.0 * abs(math.sin(theta / 2.0))) if abs(theta) > 1e-9 else c
    mx, my = (x0 + x1) / 2.0, (y0 + y1) / 2.0
    h = math.sqrt(max(0.0, r * r - (c / 2.0) ** 2))
    nx, ny = -(y1 - y0) / c, (x1 - x0) / c          # 法向单位向量
    sign = 1.0 if bulge > 0 else -1.0
    cx, cy = mx + nx * h * sign, my + ny * h * sign
    a0 = math.atan2(y0 - cy, x0 - cx)
    a1 = math.atan2(y1 - cy, x1 - cx)
    if sign > 0 and a1 < a0: a1 += 2 * math.pi
    if sign < 0 and a1 > a0: a1 -= 2 * math.pi
    n = max(3, int(abs(theta) / (math.pi / 48)))
    return [(cx + r * math.cos(a0 + (a1 - a0) * i / n),
             cy + r * math.sin(a0 + (a1 - a0) * i / n)) for i in range(n + 1)]

def draw_text(font, text, path, cell=140):
    # 每个字形单独摆一格，只验证解码正确性
    canvas = Canvas(cell*len(text), cell)
    for idx, ch in enumerate(text):
        g = font.glyphs.get(ord(ch))
        if g is None: continue
        segs = decode(g, font)['segs']
        # 归一化
        pts = []
        for s in segs:
            if s[1]:
                pts += [(s[0][0][0], s[0][0][1]), (s[0][1][0], s[0][1][1])]
            else:
                pts += s[0]
        if not pts: continue
        minx = min(p[0] for p in pts); maxx = max(p[0] for p in pts)
        miny = min(p[1] for p in pts); maxy = max(p[1] for p in pts)
        k = (cell*0.72) / max(1e-6, max(maxx-minx, maxy-miny))
        ox, oy = idx*cell + cell*0.14, cell*0.86
        for s in segs:
            if s[1]:
                if len(s[0]) == 3:
                    pts2 = bulge_points(s[0][0], s[0][1], s[0][2])
                else:
                    pts2 = list(s[0])
                prev = None
                for p in pts2:
                    q = (ox + (p[0]-minx)*k, oy - (p[1]-miny)*k)
                    if prev: canvas.line(prev[0], prev[1], q[0], q[1])
                    prev = q
            else:
                prev = None
                for p in s[0]:
                    q = (ox + (p[0]-minx)*k, oy - (p[1]-miny)*k)
                    if prev: canvas.line(prev[0], prev[1], q[0], q[1])
                    prev = q
    canvas.png(path)
    return path

if __name__ == '__main__':
    f = ShxFont(r"C:\Program Files\Autodesk\AutoCAD 2024\Fonts\txt.shx")
    print("kind=%s num=%d glyphs=%d trailing=%d" % (f.kind, f.num, len(f.glyphs), f.trailing))
    codes = sorted(f.glyphs)
    contig = (codes == list(range(codes[0], codes[-1] + 1)))
    print("code range 0x%X..0x%X contiguous=%s" % (codes[0], codes[-1], contig))
    g0 = f.glyphs[0]
    print("glyph0 raw[0:16]:", g0[:16].hex(' '), "| name:", split_name(g0)[1])
    for ch in 'ABO0g':
        g = f.glyphs.get(ord(ch))
        if g:
            st = {}
            segs = decode(g, f, stats=st)['segs']
            print("glyph '%s' len=%d segs=%d ops=%s" % (ch, len(g), len(segs), st))
    out = draw_text(f, "ABO0gQ", r"C:\temp\shx_test.png")
    print("png:", out, os.path.getsize(out))
