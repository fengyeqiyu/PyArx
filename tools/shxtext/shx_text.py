"""SHX 文字 -> 几何轮廓：Unicode 文字按实测字形包围盒排版，输出 JSON / DXF / PNG"""
import struct, sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from shx_decode import ShxFont, decode, Canvas, bulge_points, split_name

FONT = r"C:\Program Files\Autodesk\AutoCAD 2024\Fonts"

def contours_of(font, code):
    """单个字形：返回 [(points)] 原始单位轮廓"""
    g = font.glyphs.get(code)
    if g is None: return None
    res = decode(g, font)
    out = []
    for s in res['segs']:
        if s[1] and len(s[0]) == 3:
            pts = bulge_points(s[0][0], s[0][1], s[0][2])
        else:
            pts = list(s[0]) if s[1] else s[0]
        if len(pts) >= 2:
            out.append([(p[0], p[1]) for p in pts])
    return out

def bbox(cs):
    xs = [p[0] for c in cs for p in c]
    ys = [p[1] for c in cs for p in c]
    return min(xs), max(xs), min(ys), max(ys)

class FontProfile:
    """字体的实测度量：方向(y 是否向下)、设计高、字宽策略"""
    def __init__(self, font, kind):
        self.font = font
        self.kind = kind
        self.samples = []
        codes = [c for c in font.glyphs if c != 0 and (self.kind != 'unifont' or 32 <= c <= 126)]
        ymax_pos, ymax_neg, minys = 0.0, 0.0, []
        for c in codes:
            cs = contours_of(font, c)
            if not cs: continue
            b = bbox(cs)
            if b[1] - b[0] < 0.5 or b[3] - b[2] < 0.5: continue
            self.samples.append((c, b))
            ymax_pos = max(ymax_pos, b[3])
            ymax_neg = max(ymax_neg, -b[2])
            minys.append(b[2])
        neg = sum(1 for v in minys if v <= -1)
        self.y_down = neg > len(minys) * 0.7
        # 纵向：取样本整体跨度归一化，最低点对齐基线
        self.miny = min(b[2] for _, b in self.samples)
        self.maxy = max(b[3] for _, b in self.samples)
        span = self.maxy - self.miny
        hdr = 0
        g0 = font.glyphs.get(0)
        if g0:
            body = split_name(g0)[0]
            hdr = body[0] if body else 0
        self.design_h = hdr if 2 < hdr < span * 1.2 else span
        self.span = span or 1.0
        self.cjk = (kind == 'bigfont')

    def glyph(self, ch, height, pen_x):
        """返回 (轮廓列表[已定位], 字宽)"""
        if self.kind == 'bigfont':
            try:
                b = ch.encode('gb2312')
            except UnicodeEncodeError:
                return None, None
            if len(b) != 2:
                return None, None
            code = (b[0] << 8) | b[1]
        else:
            code = ord(ch)
        cs = contours_of(self.font, code)
        if not cs: return None, None
        s = height / self.span
        b = bbox(cs)
        out = []
        for c in cs:
            pts = []
            for x, y in c:
                pts.append((pen_x + (x - b[0]) * s, (y - self.miny) * s))
            out.append(pts)
        if self.cjk:
            adv = height
        else:
            adv = (b[1] - b[0]) * s + height * 0.06
        return out, adv

def build(text, primary, big=None, height=100.0, line_gap=1.9):
    p = FontProfile(primary, primary.kind)
    b = FontProfile(big, 'bigfont') if big else None
    contours, info = [], []
    x = y = 0.0
    for ch in text:
        if ch == '\n':
            x = 0.0; y -= height * line_gap; continue
        got = None
        for prof in (p, b):
            if prof is None: continue
            cs, adv = prof.glyph(ch, height, x)
            if cs:
                got = (prof, cs, adv); break
        if not got:
            x += height * 0.5; continue
        prof, cs, adv = got
        if y:
            cs = [[(px, py + y) for px, py in c] for c in cs]
        contours.extend(cs)
        info.append({'char': ch, 'font': prof.kind, 'advance': round(adv, 2),
                     'contours': len(cs), 'points': sum(len(c) for c in cs)})
        x += adv
    return contours, info

def write_json(cs, path):
    open(path, 'w', encoding='utf-8').write(json.dumps(
        {'contours': [[[round(p[0], 3), round(p[1], 3)] for p in c] for c in cs]}, ensure_ascii=False))

def write_dxf(cs, path, layer='0'):
    """最小 R12 DXF（HEADER + ENTITIES），每段一条 LINE"""
    out = ["0", "SECTION", "2", "HEADER",
           "9", "$ACADVER", "1", "AC1009",
           "0", "ENDSEC",
           "0", "SECTION", "2", "ENTITIES"]
    for c in cs:
        for i in range(len(c) - 1):
            out += ["0", "LINE", "8", layer]
            for p in (c[i], c[i + 1]):
                out += ["10", "%.4f" % p[0], "20", "%.4f" % p[1], "30", "0.0"]
    out += ["0", "ENDSEC", "0", "EOF"]
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')

def preview(cs, path, w=1600, h=560, pad=32):
    xs = [p[0] for c in cs for p in c]; ys = [p[1] for c in cs for p in c]
    minx, maxx, miny, maxy = min(xs), max(xs), min(ys), max(ys)
    k = min((w - 2 * pad) / max(1e-6, maxx - minx), (h - 2 * pad) / max(1e-6, maxy - miny))
    cv = Canvas(w, h)
    for c in cs:
        prev = None
        for p in c:
            q = (pad + (p[0] - minx) * k, h - pad - (p[1] - miny) * k)
            if prev: cv.line(prev[0], prev[1], q[0], q[1])
            prev = q
    cv.png(path)

if __name__ == '__main__':
    en = ShxFont(os.path.join(FONT, 'txt.shx'))
    zh = ShxFont(os.path.join(FONT, 'gbcbig.shx'))
    pe, pz = FontProfile(en, 'unifont'), FontProfile(zh, 'bigfont')
    print("txt    y向下=%s 设计高=%.1f 样本=%d" % (pe.y_down, pe.design_h, len(pe.samples)))
    print("gbcbig y向下=%s 设计高=%.1f 样本=%d" % (pz.y_down, pz.design_h, len(pz.samples)))
    text = "PyArx ShxText 0123 Aa gq\n文字转线条 工程汉字 中"
    cs, info = build(text, en, zh, height=100.0)
    print("轮廓=%d 字形=%d 顶点=%d" % (len(cs), len(info), sum(len(c) for c in cs)))
    for r in info[:4] + info[-4:]:
        print("   %s %-8s adv=%6.1f contours=%d pts=%d"
              % (r['char'], r['font'], r['advance'], r['contours'], r['points']))
    write_json(cs, r"C:\temp\shx_text.json")
    write_dxf(cs, r"C:\temp\shx_text.dxf")
    preview(cs, r"C:\temp\shx_text.png")
    for f in ('shx_text.json', 'shx_text.dxf', 'shx_text.png'):
        print("  %s %d bytes" % (f, os.path.getsize(r"C:\temp\%s" % f)))
