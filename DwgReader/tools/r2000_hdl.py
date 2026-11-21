import sys

d = open(r'G:\PyArx\samples\dwg_versions\v_2000.dwg', 'rb').read()


class R:
    def __init__(self, a, pos=0):
        self.a = a
        self.p = pos

    def b(self, n=1):
        v = 0
        for _ in range(n):
            g = self.p
            v = (v << 1) | ((d[self.a + g // 8] >> (7 - g % 8)) & 1)
            self.p += 1
        return v

    def rc(self):
        return self.b(8)

    def rs(self):
        lo = self.rc()
        return (self.rc() << 8) | lo

    def bs(self):
        t = self.b(2)
        return self.rs() if t == 0 else self.b(8) if t == 1 else 0 if t == 2 else 256

    def bl(self):
        n = self.rc()
        v = 0
        for i in range(n):
            v |= self.rc() << (8 * i)
        return v

    def bd(self):
        t = self.b(2)
        if t == 1:
            return 1.0
        if t == 0:
            lo = self.rs()
            hi = self.rs()
            import struct
            return struct.unpack('<d', struct.pack('<II', lo, hi))[0]
        return 0.0

    def href(self):
        c = self.rc()
        n = c & 15
        v = 0
        for _ in range(n):
            v = (v << 8) | self.rc()
        return c >> 4, n, v


def header(a):
    r = R(a)
    size = r.rs()
    t = r.bs()
    bs = r.rs() | (r.rs() << 16)
    h = r.href()
    return size, t, bs, h, r.p


def try_common(a, start, end):
    r = R(a, start)
    out = []
    pv = r.b()
    out.append(f'pv={pv}')
    if pv:
        return None
    em = r.b(2)
    nr = r.bs()
    if nr > 100:
        return None
    nol = r.b()
    col = r.bs()
    if col > 256:
        return None
    lt = r.bd()
    if not (0.01 <= lt <= 100 or lt == 0):
        return None
    lf = r.b(2)
    pf = r.b(2)
    inv = r.bs()
    if inv > 3:
        return None
    lw = r.rc()
    if lw > 31 and lw != 0xFF and lw not in (0x1F,):
        pass
    out.append(f'em={em} nr={nr} nol={nol} col={col} lt={lt} lf={lf} pf={pf} inv={inv} lw={lw} end={r.p}')
    return out, r.p, em, nol, lf, pf


def try_handles(a, start, bodyend, em, nol, lf, pf):
    r = R(a, start)
    refs = []
    try:
        if em == 0:
            refs.append(('owner', r.href()))
        refs.append(('xdic', r.href()))
        refs.append(('layer', r.href()))
        if not nol:
            refs.append(('prev', r.href()))
            refs.append(('next', r.href()))
        if lf == 3:
            refs.append(('ltype', r.href()))
        if pf == 3:
            refs.append(('plotstyle', r.href()))
    except Exception:
        return None
    ok = all(n <= 4 and c <= 12 for _, (c, n, v) in refs)
    if not ok:
        return None
    if r.p > bodyend or r.p < bodyend - 16:
        return None
    return refs, r.p


for a in [int(x) for x in sys.argv[1:]] or [130938, 131222, 14194985]:
    size, t, bs, h, hdr_end = header(a)
    msbits = 16
    bodyend = msbits + size * 8
    print(f'== addr={a} size={size} type={t} bitsize={bs} handle={h[2]} hdrEnd={hdr_end} bodyEnd={bodyend}')
    for cstart in range(hdr_end - 2, hdr_end + 48):
        cm = try_common(a, cstart, bodyend)
        if not cm:
            continue
        cout, cend, em, nol, lf, pf = cm
        print(f'  common[{cstart}] ' + ' '.join(cout))
        for base in (msbits + bs, 26 + bs, hdr_end + bs, cend + bs, msbits + bs - 16):
            refs = try_handles(a, base, bodyend, em, nol, lf, pf)
            if refs:
                rr, hend = refs
                print(f'    HANDLES OK base={base} end={hend} pad={bodyend-hend}: ' +
                      ' '.join(f'{k}=c{c}/n{n}/{v}' for k, (c, n, v) in rr))