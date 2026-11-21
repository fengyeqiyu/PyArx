# -*- coding: utf-8 -*-
"""自校验：候选地址处对象头里的 handle 是否等于句柄表给出的 handle。"""
HDL = r"G:\PyArx\examples\cad2d\dwg1021\hdl_full.bin"
OBJ = r"G:\PyArx\examples\cad2d\dwg1021\obj_full.bin"


def umc(b, i, n):
    r = 0; j = 0
    for _ in range(8):
        c = b[i]; i += 1
        if not (c & 0x80):
            return r | (c << j), i
        r |= (c & 0x7F) << j; j += 7
    raise ValueError


def mc(b, i, n):
    r = 0; j = 0
    for _ in range(5):
        c = b[i]; i += 1
        if not (c & 0x80):
            neg = bool(c & 0x40)
            if neg:
                c &= 0xBF
            r |= c << j
            return (-r if neg else r), i
        r |= (c & 0x7F) << j; j += 7
    raise ValueError


b = open(HDL, "rb").read()
n = len(b); i = 0
entries = []
while i + 2 <= n:
    start = i
    size = (b[i] << 8) | b[i + 1]
    if size <= 2:
        break
    i += 2
    end = start + size
    last_h = last_o = 0
    while i < end:
        dh, i = umc(b, i, n)
        do, i = mc(b, i, n)
        last_h += dh; last_o += do
        entries.append((last_h, last_o))
    i = end + 2

o = open(OBJ, "rb").read()


class BS:
    def __init__(self, buf, pos):
        self.b = buf; self.p = pos; self.bit = 0

    def bit1(self):
        v = (self.b[self.p] >> (7 - self.bit)) & 1
        self.bit += 1
        if self.bit == 8:
            self.bit = 0; self.p += 1
        return v

    def bb(self):
        return (self.bit1() << 1) | self.bit1()

    def rc(self):
        if self.bit == 0:
            v = self.b[self.p]; self.p += 1
            return v
        v = ((self.b[self.p] << self.bit) & 0xFF) | (self.b[self.p + 1] >> (8 - self.bit))
        self.p += 1
        return v

    def rs(self):
        hi = self.rc(); lo = self.rc()
        return (hi << 8) | lo

    def rl(self):
        lo = self.rs(); hi = self.rs()
        return (hi << 16) | lo

    def handle(self):
        c = self.rc(); sz = c & 0x0F; v = 0
        for _ in range(sz):
            v = (v << 8) | self.rc()
        return v


def obj_handle(addr):
    if addr < 0 or addr + 6 >= len(o):
        return None
    try:
        q = BS(o, addr)
        c = q.bb()
        if c == 0:
            q.rs()
        elif c == 1:
            q.rc()
        elif c == 3:
            q.rs()
        q.rl()          # bitsize
        return q.handle()
    except Exception:
        return None


bad2 = []
for h, off in entries:
    got = obj_handle(off + 2)
    if got != h:
        bad2.append((h, off, got))
print("total", len(entries), "match@+2", len(entries) - len(bad2), "fail", len(bad2))
for h, off, got in bad2[:12]:
    print(f"  fail h={h} off={off} got@+2={got} got@+4={obj_handle(off + 4)} got@+0={obj_handle(off)}")
