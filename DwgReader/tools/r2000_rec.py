import sys

d = open(r'G:\PyArx\samples\dwg_versions\v_2000.dwg', 'rb').read()
a = int(sys.argv[1])


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
        lo = self.b(8)
        hi = self.b(8)
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


r = R(a)
ms = r.rs()  # 近似：MS 前两字节
print(f'pos0 bits {r.pos()} ms~{ms}')
r = R(a)
size = r.rs()
print(f'size={size} @{r.pos()}')
t = r.bs()
print(f'type={t} @{r.pos()} (typeEnd={r.pos()})')
bs_pos = r.pos()
bitsize = r.rl()
print(f'bitsize={bitsize} @{r.pos()}')
h = r.h()
print(f'handle={h} @{r.pos()}')
print(f'afterHeader={r.pos()} dataBitsFromTypeEnd={r.pos()-bs_pos}')
print(f'hdlStart = {bs_pos + bitsize} = byte {int((bs_pos+bitsize)//8)}.{(bs_pos+bitsize)%8}')
print('bytes', ' '.join(f'{x:02X}' for x in d[a:a + 40]))
