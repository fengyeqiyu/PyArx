# -*- coding: utf-8 -*-
"""纯 Python SHX 字体渲染器（脱离 AutoCAD / OGS / ShFontParser.dll）。

用法：
    f  = ShxFont(r"...\\romans.shx")
    st = ShxTextStyle(f, bigfont=ShxFont(r"...\\hztxt.shx"))
    polys = st.layout("图纸 Plan 2026", height=10.0)     # -> [ [(x,y),...], ... ]  世界坐标
    save_svg(polys, "out.svg", height=10.0, padding=2.0)
    save_png(polys, "out.png", height=10.0, px_per_unit=20)

坐标单位：与 CAD 一致，y 轴向上，原点在文本基线起点。
"""

import math
import os
import struct
import zlib

__all__ = ["ShxFont", "ShxTextStyle", "save_svg", "save_png", "polyline_bbox"]

# ---------------------------------------------------------------- 常量
TAN22_5 = math.tan(math.radians(22.5))     # 0.414213...
OCT = math.pi / 4.0                        # 45°


# ---------------------------------------------------------------- 容器解析
class ShxFont:
    """解析一个 .shx 文件：头部 → 字形表 → 延迟解码。"""

    UNIFONT_HDR = b"AutoCAD-86 unifont"
    BIGFONT_HDR = b"AutoCAD-86 bigfont"
    SHAPES_HDR = b"AutoCAD-86 shapes"

    def __init__(self, path):
        self.path = path
        self.name = os.path.basename(path)
        with open(path, "rb") as fp:
            self.data = fp.read()
        self.kind = self._detect_kind()
        self._raw = {}          # code -> 字形字节串
        self._cache = {}        # code -> Glyph
        self._parse()
        self._advance_default = None

    # -- 头部
    def _detect_kind(self):
        head = self.data[:40]
        if self.UNIFONT_HDR in head:
            return "unifont"
        if self.BIGFONT_HDR in head:
            return "bigfont"
        if self.SHAPES_HDR in head:
            return "shapes"
        return "unknown"

    def _parse(self):
        d = self.data
        n = len(d)
        if self.kind == "unifont":
            # header 25B("AutoCAD-86 unifont 1.0\r\n\x1a") + u16 count @25，记录 @27
            ofs = d.index(b"\x1a", 0, 40) + 1 if b"\x1a" in d[:40] else 25
            self.count = struct.unpack_from("<H", d, ofs)[0]
            p = ofs + 2
            got = 0
            while p + 4 <= n and got < self.count:
                code, ln = struct.unpack_from("<HH", d, p)
                if ln == 0 or p + 4 + ln > n:
                    break
                self._raw[code] = d[p + 4:p + 4 + ln]
                p += 4 + ln
                got += 1
            self.trailing = n - p
        elif self.kind == "bigfont":
            # header 27B + u16 count @27 + u16 nchange @29 + 4B/change + 8B/索引
            self.count = struct.unpack_from("<H", d, 27)[0]
            nchange = struct.unpack_from("<H", d, 29)[0]
            table = 31 + 4 * nchange
            for i in range(self.count):
                b = table + i * 8
                if b + 8 > n:
                    break
                code, ln, off = struct.unpack_from("<HHI", d, b)
                if off and ln and off + ln <= n:
                    self._raw[code] = d[off:off + ln]
            self.trailing = 0
        else:                       # shapes / unknown：无法解析
            self.count = 0
            self.trailing = n

    # -- 查询
    def __len__(self):
        return len(self._raw)

    def __contains__(self, code):
        return code in self._raw

    def lookup(self, code):
        """按字符取字形字节串；支持 unifont 空格缺省。"""
        if code in self._raw:
            return self._raw[code]
        if code == 0x20 and self.kind == "unifont":
            return b""
        return None

    # -- 解码（带缓存）
    def glyph(self, code, depth=0):
        if code in self._cache:
            return self._cache[code]
        rec = self.lookup(code)
        g = Glyph() if rec is None else self._decode_record(rec, depth)
        self._cache[code] = g
        return g

    def _decode_record(self, rec, depth):
        # 记录以 name(0x00 结尾) 开头，字体字形 name 为空
        i = 0
        while i < len(rec) and rec[i] != 0:
            i += 1
        body = rec[i + 1:]
        return self._run(body, depth)

    # -- 指令解释器
    def _run(self, code, depth):
        g = Glyph()
        pos = [0.0, 0.0]
        pen = False
        scale = 1.0
        stack = []
        cur = []                     # 当前笔画点列
        i, n = 0, len(code)
        self._maxx = 0.0

        def flush():
            if cur and len(cur) > 1:
                g.polylines.append(list(cur))
            cur.clear()

        def move_to(nx, ny, draw):
            if draw:
                if not cur:
                    cur.append((pos[0], pos[1]))
                cur.append((nx, ny))
            else:
                flush()
            pos[0], pos[1] = nx, ny
            if nx > g.advance:
                g.advance = nx
            if ny > g.ymax:
                g.ymax = ny
            if ny < g.ymin:
                g.ymin = ny

        while i < n:
            b = code[i]
            if b == 0:               # 结束
                break
            L, d = b >> 4, b & 0x0F
            if L:
                # 普通向量：L=长度1..15, d=方向0..15
                vx, vy = self._vec(d, L * scale)
                move_to(pos[0] + vx, pos[1] + vy, pen)
                i += 1
                continue

            # ---- 命令（b < 0x10）
            op = d
            if op == 1:
                pen = True
                i += 1
            elif op == 2:
                pen = False
                i += 1
            elif op == 3:             # 缩放：除以参数
                v = code[i + 1] if i + 1 < n else 1
                scale /= (v or 1)
                i += 2
            elif op == 4:             # 缩放：乘以参数
                v = code[i + 1] if i + 1 < n else 1
                scale *= (v or 1)
                i += 2
            elif op == 5:
                stack.append(tuple(pos))
                i += 1
            elif op == 6:
                if stack:
                    p = stack.pop()
                    pos[0], pos[1] = p
                flush()
                i += 1
            elif op == 7:             # 引用子 shape（不推进笔位置）
                sub = code[i + 1] if i + 1 < n else 0
                i += 2
                sg = self.glyph(sub, depth + 1) if depth < 3 else Glyph()
                if sg.polylines:
                    if pen:
                        if not cur:
                            cur.append((pos[0], pos[1]))
                        for pl in sg.polylines:
                            cur.extend((pos[0] + x, pos[1] + y) for x, y in pl)
                    else:
                        flush()
                        for pl in sg.polylines:
                            g.polylines.append([(pos[0] + x, pos[1] + y) for x, y in pl])
            elif op == 8:             # 有符号位移 (dx,dy)
                if i + 2 >= n:
                    break
                dx, dy = _s8(code, i + 1), _s8(code, i + 2)
                i += 3
                move_to(pos[0] + dx * scale, pos[1] + dy * scale, pen)
            elif op == 9:             # 连续位移直到 (0,0)
                i += 1
                while i + 1 < n:
                    dx, dy = _s8(code, i), _s8(code, i + 1)
                    i += 2
                    if dx == 0 and dy == 0:
                        break
                    move_to(pos[0] + dx * scale, pos[1] + dy * scale, pen)
            elif op == 10:            # 八分弧: 半径, 起始/跨度
                if i + 2 >= n:
                    break
                r = code[i + 1] * scale
                sc = code[i + 2]
                i += 3
                start = (sc >> 4) & 0x07
                span = sc & 0x07 or 8
                cw = bool(sc & 0x80)
                a0 = start * OCT
                sweep = span * OCT * (-1.0 if cw else 1.0)
                pts = _arc(pos, r, a0, a0 + sweep, r)
                if pen:
                    if not cur:
                        cur.append((pos[0], pos[1]))
                    cur.extend(pts)
                    move_to(pts[-1][0], pts[-1][1], True)
                else:
                    move_to(pts[-1][0], pts[-1][1], False)
            elif op == 11:            # 分数弧: s_off,e_off,r_hi,r_lo,sc
                if i + 5 >= n:
                    break
                so, eo = code[i + 1], code[i + 2]
                r = (code[i + 3] * 256 + code[i + 4]) * scale
                sc = code[i + 5]
                i += 6
                start = (sc >> 4) & 0x07
                span = sc & 0x07 or 8
                cw = bool(sc & 0x80)
                a0 = start * OCT + OCT * so / 256.0
                a1 = start * OCT + span * OCT + OCT * eo / 256.0
                if cw:
                    a0, a1 = a1, a0
                pts = _arc(pos, r, a0, a1, abs(a1 - a0) / (2 * math.pi) * r)
                if pen:
                    if not cur:
                        cur.append((pos[0], pos[1]))
                    cur.extend(pts)
                    move_to(pts[-1][0], pts[-1][1], True)
                else:
                    move_to(pts[-1][0], pts[-1][1], False)
            elif op == 12:            # 凸度弧 (dx,dy,bulge)
                if i + 3 >= n:
                    break
                dx, dy = _s8(code, i + 1), _s8(code, i + 2)
                bulge = _s8(code, i + 3) / 127.0
                i += 4
                p0 = (pos[0], pos[1])
                p1 = (pos[0] + dx * scale, pos[1] + dy * scale)
                pts = _bulge(p0, p1, bulge)
                if pen:
                    if not cur:
                        cur.append(p0)
                    cur.extend(pts[1:])
                    move_to(p1[0], p1[1], True)
                else:
                    move_to(p1[0], p1[1], False)
            elif op == 13:            # 多凸度弧直到 (0,0)
                i += 1
                while i + 2 < n:
                    dx, dy = _s8(code, i), _s8(code, i + 1)
                    bulge = _s8(code, i + 2) / 127.0
                    i += 3
                    if dx == 0 and dy == 0:
                        break
                    p0 = (pos[0], pos[1])
                    p1 = (pos[0] + dx * scale, pos[1] + dy * scale)
                    pts = _bulge(p0, p1, bulge)
                    if pen:
                        if not cur:
                            cur.append(p0)
                        cur.extend(pts[1:])
                        move_to(p1[0], p1[1], True)
                    else:
                        move_to(p1[0], p1[1], False)
            else:                     # 14 / 其它：无参数，忽略
                i += 1

        flush()
        return g

    # -- 16 方向向量表：长度沿主轴，对角方向 dx=dy=L（AutoCAD 约定）
    _VEC = (
        (1.0, 0.0), (1.0, TAN22_5), (1.0, 1.0), (TAN22_5, 1.0),
        (0.0, 1.0), (-TAN22_5, 1.0), (-1.0, 1.0), (-1.0, TAN22_5),
        (-1.0, 0.0), (-1.0, -TAN22_5), (-1.0, -1.0), (-TAN22_5, -1.0),
        (0.0, -1.0), (TAN22_5, -1.0), (1.0, -1.0), (1.0, -TAN22_5),
    )

    @classmethod
    def _vec(cls, direction, length):
        ux, uy = cls._VEC[direction & 0x0F]
        return (ux * length, uy * length)


# ---------------------------------------------------------------- 字形对象
class Glyph:
    __slots__ = ("polylines", "advance", "ymax", "ymin")

    def __init__(self):
        self.polylines = []
        self.advance = 0.0
        self.ymax = 0.0
        self.ymin = 0.0

    def __repr__(self):
        return "<Glyph %d pl, adv=%.2f>" % (len(self.polylines), self.advance)


def _s8(buf, i):
    v = buf[i]
    return v - 256 if v >= 128 else v


def _arc(center, r, a0, a1, arc_len):
    """按弧长自适应离散（步长 ≈ 1/24 周）。"""
    if r <= 0:
        return [tuple(center)]
    n = max(4, int(abs(a1 - a0) / (2 * math.pi) * 48))
    return [(center[0] + r * math.cos(a0 + (a1 - a0) * k / n),
             center[1] + r * math.sin(a0 + (a1 - a0) * k / n)) for k in range(n + 1)]


def _bulge(p0, p1, bulge):
    """凸度 → 折线；bulge = tan(θ/4)。"""
    if abs(bulge) < 1e-9:
        return [p0, p1]
    x0, y0 = p0
    x1, y1 = p1
    c = math.hypot(x1 - x0, y1 - y0)
    if c < 1e-9:
        return [p0, p1]
    theta = 4.0 * math.atan(bulge)
    r = abs(c / (2.0 * math.sin(theta / 2.0)))
    mx, my = (x0 + x1) / 2.0, (y0 + y1) / 2.0
    h = math.sqrt(max(0.0, r * r - (c / 2.0) ** 2))
    nx, ny = -(y1 - y0) / c, (x1 - x0) / c
    sign = 1.0 if bulge > 0 else -1.0
    cx, cy = mx + nx * h * sign, my + ny * h * sign
    a0 = math.atan2(y0 - cy, x0 - cx)
    a1 = math.atan2(y1 - cy, x1 - cx)
    if sign > 0 and a1 < a0:
        a1 += 2 * math.pi
    if sign < 0 and a1 > a0:
        a1 -= 2 * math.pi
    n = max(4, int(abs(theta) / (math.pi / 48.0)))
    return [(cx + r * math.cos(a0 + (a1 - a0) * k / n),
             cy + r * math.sin(a0 + (a1 - a0) * k / n)) for k in range(n + 1)]


# ---------------------------------------------------------------- 文本样式
class ShxTextStyle:
    """西文 unifont + 中文 bigfont 组合，模拟 CAD 的 shx 文字样式。

    度量自动探测：西文取 A-Z 的 ymin 中位数当基线、ymax 中位数当 cap 高；
    中文取常用汉字的 bbox 中位数。height 参数 = 大写字高（cap height）。
    """

    WEST_PROBE = range(65, 91)          # A-Z
    CJK_PROBE = "中国图建设计说一"

    def __init__(self, font, bigfont=None, gap_ratio=0.15):
        self.font = font
        self.bigfont = bigfont
        self.gap_ratio = gap_ratio
        self.base_w, self.cap_w = _metrics(font, [chr(c) for c in self.WEST_PROBE])
        self.base_c, self.cap_c = (_metrics(bigfont, list(self.CJK_PROBE))
                                   if bigfont is not None else (0.0, 21.0))

    # -- 字符 → (字形, 度量)
    def pick(self, ch):
        o = ord(ch)
        if o < 0x80:
            g = self.font.glyph(o)
            return g, (self.base_w, self.cap_w, 0.0)
        if self.bigfont is None:
            return None, None
        try:
            gb = ch.encode("gb2312")
        except UnicodeEncodeError:
            return None, None
        if len(gb) == 1:
            return self.font.glyph(gb[0]), (self.base_w, self.cap_w, 0.0)
        code = gb[0] * 256 + gb[1]
        g = self.bigfont.glyph(code)
        if not g.polylines and code not in self.bigfont:
            return self.font.glyph(gb[0]), (self.base_w, self.cap_w, 0.0)
        return g, (self.base_c, self.cap_c, self.height_ratio())

    def height_ratio(self):
        return 1.0

    # -- 排版
    def layout(self, text, height=10.0, origin=(0.0, 0.0), width_factor=1.0,
               oblique=0.0, spacing=0.0, rotation=0.0, align="left"):
        """返回世界坐标折线列表；y 向上，基线 y=0，height = 大写字高。"""
        height = float(height)
        tan_ob = math.tan(math.radians(oblique)) if oblique else 0.0

        items = []
        pen = 0.0
        for ch in text:
            if ch == "\n":
                items.append(("nl", None))
                pen = 0.0
                continue
            g, met = self.pick(ch)
            if g is None or not g.polylines:
                pen += height * 0.5 + spacing
                continue
            xs = [p[0] for pl in g.polylines for p in pl]
            xmin, xmax = min(xs), max(xs)
            base, cap, cjk = met[0], met[1], met[2]
            s = height / cap if cap > 1e-6 else 1.0
            if cjk:
                # 中文：全角 cell，保留字形在 cell 内的原始左右位置
                items.append(("g", (g, pen, 0.0, base, s)))
                pen += height + spacing
            else:
                items.append(("g", (g, pen, xmin, base, s)))
                pen += (xmax - xmin) * s + self.gap_ratio * height + spacing

        total = pen
        dx0 = -total / 2.0 if align == "center" else (-total if align == "right" else 0.0)

        cos_r = math.cos(math.radians(rotation))
        sin_r = math.sin(math.radians(rotation))
        out = []
        for kind, payload in items:
            if kind != "g":
                continue
            g, gx, xmin, base, s = payload
            for pl in g.polylines:
                pts = []
                for x, y in pl:
                    ux = ((x - xmin) * s + gx + dx0) * width_factor + (y - base) * s * tan_ob
                    uy = (y - base) * s
                    pts.append((origin[0] + ux * cos_r - uy * sin_r,
                                origin[1] + ux * sin_r + uy * cos_r))
                out.append(pts)
        return out


def _metrics(font, chars):
    """探测字体的基线 y 与 cap 高。"""
    if font is None:
        return (0.0, 21.0)
    pairs = []
    for ch in chars:
        try:
            gb = ch.encode("gb2312")
        except UnicodeEncodeError:
            gb = None
        if ord(ch) < 0x80:
            g = font.glyph(ord(ch))
        elif gb is not None and len(gb) == 2:
            g = font.glyph(gb[0] * 256 + gb[1])
        else:
            continue
        if g.polylines:
            py = [p[1] for pl in g.polylines for p in pl]
            pairs.append((min(py), max(py)))
    if not pairs:
        return (0.0, 21.0)
    pairs.sort()
    bases = sorted(p[0] for p in pairs)
    tops = sorted(p[1] for p in pairs)
    base = bases[len(bases) // 2]
    cap = tops[len(tops) // 2] - base
    if cap <= 1e-6:
        cap = 21.0
    return (base, cap)


# ---------------------------------------------------------------- 输出
def polyline_bbox(polys):
    xs = [p[0] for pl in polys for p in pl]
    ys = [p[1] for pl in polys for p in pl]
    if not xs:
        return (0.0, 0.0, 0.0, 0.0)
    return (min(xs), min(ys), max(xs), max(ys))


def save_svg(polys, path, height=None, padding=0.1, stroke_width=None):
    x0, y0, x1, y1 = polyline_bbox(polys)
    h = height or max(1e-6, y1 - y0)
    pad = h * padding
    w = max(1e-6, x1 - x0) + 2 * pad
    hh = h + 2 * pad
    sw = stroke_width or h * 0.055
    lines = []
    for pl in polys:
        pts = " ".join("%.3f,%.3f" % (p[0] - x0 + pad, (y1 - p[1]) + pad) for p in pl)
        lines.append("    <polyline points=\"%s\"/>" % pts)
    body = "\n".join(lines)
    svg = (
        '<?xml version="1.0" encoding="UTF-8"?>\n'
        '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
        'viewBox="0 0 %.3f %.3f" stroke="#111" stroke-width="%.3f" fill="none" '
        'stroke-linejoin="round" stroke-linecap="round">\n%s\n</svg>\n'
        % (int(w * 40), int(hh * 40), w, hh, sw, body)
    )
    with open(path, "w", encoding="utf-8") as f:
        f.write(svg)
    return path


def save_png(polys, path, px_per_unit=40, margin=None, aa=2):
    """抗锯齿光栅化（超采样 + 灰度降采样）。"""
    x0, y0, x1, y1 = polyline_bbox(polys)
    if x1 - x0 < 1e-9 and y1 - y0 < 1e-9:
        raise ValueError("empty geometry")
    margin = margin if margin is not None else (y1 - y0) * 0.12
    wu, hu = (x1 - x0) + 2 * margin, (y1 - y0) + 2 * margin
    W, H = int(wu * px_per_unit), int(hu * px_per_unit)
    SW, SH = W * aa, H * aa
    cov = bytearray(SW * SH)
    for pl in polys:
        pts = [((p[0] - x0 + margin) * px_per_unit * aa, (y1 - p[1] + margin) * px_per_unit * aa) for p in pl]
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            _fat_line(cov, SW, SH, ax, ay, bx, by, max(1, int(aa * 1.2)))
    # 降采样 + PNG
    out = bytearray(W * H)
    for y in range(H):
        for x in range(W):
            acc = 0
            for dy in range(aa):
                row = (y * aa + dy) * SW
                for dx in range(aa):
                    acc += cov[row + x * aa + dx]
            v = acc // (aa * aa)
            out[y * W + x] = 255 - min(255, v)      # 黑字白底
    raw = b"".join(b"\x00" + bytes(out[y * W:(y + 1) * W]) for y in range(H))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 0, 0, 0, 0)) \
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)
    return path


def _fat_line(cov, W, H, x0, y0, x1, y1, r):
    """画粗线段到覆盖率缓冲；r=半径像素。"""
    n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
    for k in range(n + 1):
        t = k / n if n else 0
        cx, cy = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        ix, iy = int(cx), int(cy)
        for dy in range(-r, r + 1):
            yy = iy + dy
            if yy < 0 or yy >= H:
                continue
            row = yy * W
            for dx in range(-r, r + 1):
                xx = ix + dx
                if 0 <= xx < W:
                    cov[row + xx] = 255
