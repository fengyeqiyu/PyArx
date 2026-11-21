import sys

path = r'G:\PyArx\samples\dwg_versions\v_2000.dwg'
data = open(path, 'rb').read()

# 用 classic_container 的定位方式拿 objectBase：这里先按已知头部结构粗略搜索
# 直接读 section locator 太复杂，改用参数传入
base = int(sys.argv[1]) if len(sys.argv) > 1 else 0
end = int(sys.argv[2]) if len(sys.argv) > 2 else len(data)


def bits(p):
    return ''.join(f'{x:08b}' for x in data[p:p + 8])


class R:
    def __init__(self, pos):
        self.p = pos
        self.bit = 0

    def b(self, n=1):
        v = 0
        for _ in range(n):
            byte = data[self.p]
            v = (v << 1) | ((byte >> (7 - self.bit)) & 1)
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


pos = base
n = 0
while pos + 4 < end and n < 12:
    r = R(pos)
    size = r.ms()
    t = r.bs()
    hdl = r.h()
    msbits = (r.p - pos) * 8 + r.bit
    nxt = pos + (msbits + size * 8) // 8 + 2
    print(f'rec#{n} pos={pos} size={size} type={t} handle={hdl} msbits={msbits} next={nxt} crc={data[nxt-2:nxt].hex()}')
    pos = nxt
    n += 1
