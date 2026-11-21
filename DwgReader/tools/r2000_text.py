import sys

d = open(r'G:\PyArx\samples\dwg_versions\v_2000.dwg', 'rb').read()


class R:
    def __init__(self, a, p=0):
        self.a = a
        self.p = p

    def b(self, n=1):
        v = 0
        for _ in range(n):
            v = (v << 1) | ((d[self.a + self.p // 8] >> (7 - self.p % 8)) & 1)
            self.p += 1
        return v

    def rc(self):
        return self.b(8)

    def rs(self):
        lo = self.rc()
        return (self.rc() << 8) | lo

    def rl(self):
        return self.rs() | (self.rs() << 16)

    def bs(self):
        t = self.b(2)
        return self.rs() if t == 0 else self.b(8) if t == 1 else 0 if t == 2 else 256

    def rd(self):
        u = self.rs() | (self.rs() << 16)
        lo = u & 0xFFFFFFFF
        hi = self.rl()
        import struct
        return struct.unpack('<d', struct.pack('<II', lo, hi))[0]

    def bd(self):
        t = self.b(2)
        if t == 1:
            return 1.0
        if t == 0:
            return self.rd()
        return 0.0

    def be(self):
        return self.bd(), self.bd(), self.bd()

    def bt(self):
        v = self.bd()
        return v, self.b(1)

    def dd(self, dv):
        t = self.b(2)
        if t == 3:
            return self.rd()
        if t == 2:
            return dv
        v = self.rd()
        return dv + v if t == 0 else dv - v

    def tv(self):
        n = self.bs()
        return ''.join(chr(self.rc()) for _ in range(n))


def head(a):
    r = R(a)
    size = r.rs()
    t = r.bs()
    bs = r.rl()
    c = r.rc()
    n = c & 15
    hv = 0
    for _ in range(n):
        hv = (hv << 8) | r.rc()
    return size, t, bs, hv, r.p


def common(r):
    pv = r.b()
    em = r.b(2)
    nr = r.bs()
    nol = r.b()
    col = r.bs()
    lf = r.b(2)
    if lf == 0:
        r.rs(); r.rs()
    lfl = r.b(2)
    pf = r.b(2)
    inv = r.bs()
    lw = r.rc()
    return dict(pv=pv, em=em, nr=nr, nol=nol, col=col, lf=lfl, pf=pf, inv=inv, lw=lw)


def text(a):
    size, t, bs, hv, hdr = head(a)
    r = R(a, hdr)
    eed = r.bs()
    c = common(r)
    print(f'addr={a} size={size} type={t} bitsize={bs} h={hv} eed={eed} common={c} geomStart={r.p} hdlStart={16+bs}')
    df = r.rc()
    bits = [f'dataflags=0x{df:02X}@{r.p-8}']
    marks = []
    if not (df & 1):
        e = r.rd()
        bits.append(f'elev={e:.3f}')
        marks.append(('elev', r.p))
    ix, iy = r.rd(), r.rd()
    bits.append(f'ins=({ix:.2f},{iy:.2f})')
    marks.append(('ins', r.p))
    if not (df & 2):
        ax, ay = r.dd(0), r.dd(0)
        bits.append(f'align=({ax:.2f},{ay:.2f})')
        marks.append(('align', r.p))
    ex, ey, ez = r.be()
    bits.append(f'ext=({ex:.2f},{ey:.2f},{ez:.2f})')
    marks.append(('ext', r.p))
    th, thf = r.bt()
    bits.append(f'thk={th:.2f}/{thf}')
    marks.append(('thk', r.p))
    if not (df & 4):
        bits.append(f'obl={r.rd():.3f}')
        marks.append(('obl', r.p))
    if not (df & 8):
        bits.append(f'rot={r.rd():.3f}')
        marks.append(('rot', r.p))
    hv = r.rd()
    bits.append(f'h={hv:.3f}')
    marks.append(('h', r.p))
    if not (df & 0x10):
        bits.append(f'w={r.rd():.3f}')
        marks.append(('w', r.p))
    bits.append(f'txt="{r.tv()}"')
    if not (df & 0x20):
        bits.append(f'gen={r.bs()}')
    if not (df & 0x40):
        bits.append(f'horiz={r.bs()}')
    if not (df & 0x80):
        bits.append(f'vert={r.bs()}')
    print(f'  geomEnd={r.p} (hdlStart={16+bs}, diff={r.p-(16+bs)})')
    print('  ' + ' '.join(bits))
    print('  marks ' + ' '.join(f'{k}={v}' for k, v in marks))


for x in sys.argv[1:]:
    text(int(x))
if not sys.argv[1:]:
    text(88320)