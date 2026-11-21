# -*- coding: utf-8 -*-
"""在 calc / calc+2 两处试解对象头，与 ARX 基准的 (handle, type) 对照。"""
OBJ = r"G:\PyArx\examples\cad2d\dwg1021\obj_full.bin"
REF = r"G:\PyArx\examples\cad2d\dwg1021\arx_all1021.tsv"
CASES = [1, 14, 5073, 62738, 65687, 101561]

b = open(OBJ, "rb").read()
ref = []
with open(REF, encoding="latin-1") as f:
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) < 2:
            continue
        try:
            ref.append((int(p[0]), int(p[1]), int(p[2]) if len(p) > 2 and p[2].isdigit() else -1))
        except ValueError:
            continue


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
        v = (self.bit1() << 1) | self.bit1()
        return v

    def rc(self):
        if self.bit == 0:
            v = self.b[self.p]; self.p += 1; return v
        hi = (self.b[self.p] & ((1 << (8 - self.bit)) - 1)) << self.bit
        lo = self.b[self.p + 1] >> (8 - self.bit)
        v = hi | lo
        self.p += 1
        return v & 0xFF

    def rs(self):
        return (self.rc() << 8) | self.rc()

    def rl(self):
        return (self.rs() << 16) | self.rs()

    def bs(self):
        c = self.bb()
        if c == 0:
            return self.rs()
        if c == 1:
            return self.rc()
        if c == 2:
            return 0
        return 256

    def handle(self):
        c = self.rc()
        sz = c & 0x0F
        v = 0
        for _ in range(sz):
            v = (v << 8) | self.rc()
        return v


def head(addr):
    q = BS(b, addr)
    t = q.bs()
    bits = q.rl()
    h = q.handle()
    return t, bits, h, q.p


for j in CASES:
    h_ref, a_ref, t_ref = ref[j]
    calc = a_ref - 2
    print(f"#{j} ref handle={h_ref} addr={a_ref} type={t_ref}")
    for label, addr in (("calc", calc), ("ref ", a_ref)):
        try:
            t, bits, h, pos = head(addr)
            print(f"   @{label} {addr}: type={t} bitsize={bits} handle={h} (bytes consumed {pos-addr})")
        except Exception as e:
            print(f"   @{label} {addr}: parse error {e}")
