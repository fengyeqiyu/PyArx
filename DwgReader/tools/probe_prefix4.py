"""按地址序重建记录边界，定位 6 条异常记录的真实前缀长度。

DWG 位序按 libredwg：字节内从高位到低位读。
"""
import sys

OBJ = r"G:\PyArx\DwgReader\build\objects_cpp.bin"
IDX = r"G:\PyArx\DwgReader\build\index_cpp.tsv"


class Bits:
    def __init__(self, d, byte=0, bit=0):
        self.d = d
        self.byte = byte
        self.bit = bit

    def B(self, n):
        v = 0
        for _ in range(n):
            v = (v << 1) | ((self.d[self.byte] >> (7 - self.bit)) & 1)
            self.bit += 1
            if self.bit == 8:
                self.bit = 0
                self.byte += 1
        return v

    def rc(self):
        return self.B(8)

    def rs(self):
        return self.B(8) | (self.B(8) << 8)

    def rl(self):
        return self.B(8) | (self.B(8) << 8) | (self.B(8) << 16) | (self.B(8) << 24)

    def bs(self):
        two = self.B(2)
        if two == 0:
            return 0
        if two == 1:
            return self.rc()
        if two == 2:
            return self.rs()
        return self.rl()

    def handle(self):
        code = self.B(4)
        size = 4 if code < 6 else (8 if code < 10 else 0)
        if size == 0:
            return 0
        return int.from_bytes(bytes(self.B(8) for _ in range(size)), "little")


def head(data, addr):
    b = Bits(data, addr)
    t = b.bs()
    size = b.rl()
    h = b.handle()
    return t, size, h, b.byte, b.bit


def main():
    data = open(OBJ, "rb").read()
    rows = []
    with open(IDX, "r", encoding="utf-8") as f:
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) >= 3:
                rows.append((int(p[0]), int(p[1]), int(p[2])))
    by_addr = sorted(rows, key=lambda r: r[1])
    pos = {r[1]: i for i, r in enumerate(by_addr)}

    for h in (2, 17, 24950, 126809, 241085, 370591):
        ent = next((r for r in rows if r[0] == h), None)
        if not ent:
            continue
        _, addr, typ = ent
        i = pos[addr]
        prev = by_addr[i - 1] if i else None
        print("\nhandle=%d addr=%d type=%d" % (h, addr, typ))
        if prev:
            _, paddr, ptyp = prev
            pt, psize, ph, pend_byte, pend_bit = head(data, paddr)
            print("  prev handle=%d addr=%d type=%d bitsize=%d end_byte=%d%s"
                  % (ph, paddr, pt, psize, pend_byte, "+%db" % pend_bit if pend_bit else ""))
            gap = addr - pend_byte
            print("  gap=%d bytes (2=正常前缀, 4=多 2 字节)" % gap)
            print("  bytes prev_end..addr:", " ".join("%02X" % c for c in data[pend_byte:addr]))
        # 在 addr 附近逐一试探，看哪个起点能让 handle 命中
        for off in range(addr - 6, addr + 3):
            ok = head(data, off)
            flag = "  <== handle 命中" if ok[2] == h else ""
            print("   +%d: type=%d bitsize=%d handle=%d%s" % (off - addr, ok[0], ok[1], ok[2], flag))


if __name__ == "__main__":
    sys.exit(main())
