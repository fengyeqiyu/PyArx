import sys

path = r'G:\PyArx\samples\dwg_versions\v_2000.dwg'
data = open(path, 'rb').read()
base = int(sys.argv[1])
end = int(sys.argv[2])
types = {1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}


class R:
    def __init__(self, pos):
        self.p = pos
        self.bit = 0

    def b(self, n=1):
        v = 0
        for _ in range(n):
            v = (v << 1) | ((data[self.p] >> (7 - self.bit)) & 1)
            self.bit += 1
            if self.bit == 8:
                self.bit = 0
                self.p += 1
        return v

    def ms(self):
        v = self.b(16)
        if v & 0x8000:
            v = (v & 0x7FFF) | (self.b(16) << 15)
        return v

    def bs(self):
        t = self.b(2)
        if t == 0:
            return self.b(16)
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
        return v


def walk(start, count=30):
    pos = start
    recs = []
    for _ in range(count):
        r = R(pos)
        size = r.ms()
        if size <= 4 or size > 3000:
            return None
        t = r.bs()
        if t not in types:
            return None
        hdl = r.h()
        if hdl == 0:
            return None
        msbits = (r.p - pos) * 8 + r.bit
        nxt = pos + (msbits + size * 8) // 8 + 2
        crc = data[nxt - 2:nxt]
        recs.append((pos, size, t, hdl, crc.hex()))
        pos = nxt
        if pos >= end:
            break
    return recs, pos


best = None
for start in range(base, min(base + 20000, end)):
    w = walk(start, 20)
    if not w:
        continue
    recs, nxt = w
    if all(x[2] in (19, 18, 17, 27, 1, 2, 7, 4, 5, 6, 10, 3) for x in recs):
        best = (start, recs, nxt)
        break

if not best:
    print('no candidate found')
    sys.exit(1)
start, recs, nxt = best
print(f'start={start} (base+{start-base})')
for x in recs[:10]:
    print(f'  pos={x[0]} size={x[1]} type={x[2]} h={x[3]} crc={x[4]}')
print('next', nxt)
