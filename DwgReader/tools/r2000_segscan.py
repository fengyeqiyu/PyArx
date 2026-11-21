import sys

d = open(r'G:\PyArx\samples\dwg_versions\v_2000.dwg', 'rb').read()


class R:
    def __init__(self, pos):
        self.p = pos
        self.bit = 0

    def b(self, n=1):
        v = 0
        for _ in range(n):
            v = (v << 1) | ((d[self.p] >> (7 - self.bit)) & 1)
            self.bit += 1
            if self.bit == 8:
                self.bit = 0
                self.p += 1
        return v

    def pos(self):
        return self.p * 8 + self.bit

    def rs(self):
        lo = self.b(8); hi = self.b(8)
        return (hi << 8) | lo

    def rl(self):
        return self.rs() | (self.rs() << 16)

    def bs(self):
        t = self.b(2)
        if t == 0:
            return self.rs()
        if t == 1:
            return self.b(8)
        if t == 2:
            return 0
        return 256

    def h(self):
        c = self.b(8)
        n = c & 0x0F
        v = 0
        for _ in range(n):
            v = (v << 8) | self.b(8)
        return v, c >> 4, n


def head(a):
    r = R(a)
    lo = r.b(8); hi = r.b(8)
    ms = lo | (hi << 8)
    hdr = 2
    if ms >= 0x8000:
        ms = (ms & 0x7FFF) | (r.rs() << 15)
        hdr = 4
    t = r.bs()
    bitsize = r.rl()
    h = r.h()
    return ms, hdr, t, bitsize, h


a0 = int(sys.argv[1])
span = int(sys.argv[2])
want = int(sys.argv[3])
for off in range(0, span):
    a = a0 + off
    try:
        ms, hdr, t, bitsize, h = head(a)
    except IndexError:
        break
    if 0 < ms < 0x4000 and t < 100 and h[2] > 0 and h[2] < 8 and abs(h[0] - want) < 4:
        print(f'off={off:>5} addr={a} ms={ms} type={t} bitsize={bitsize} handle={h[0]} code={h[1]}')
