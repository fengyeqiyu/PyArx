"""重建 Handles 页结构，定位 6 条异常记录的页内位置。

目的：判断 +4 前缀异常是否与页边界/页首记录相关。
"""
import sys

HDL = r"G:\PyArx\DwgReader\build\hdl_cpp.bin"
TARGET = {2, 17, 24950, 126809, 241085, 370591}
ADDR_BASE = 2


def umc(d, pos):
    res = 0
    j = 0
    for _ in range(8):
        b = d[pos]
        pos += 1
        if not (b & 0x80):
            return res | (b << j), pos
        res |= (b & 0x7F) << j
        j += 7
    return 0, pos


def mc(d, pos):
    neg = 0
    res = 0
    j = 0
    for _ in range(5):
        b = d[pos]
        pos += 1
        if not (b & 0x80):
            if b & 0x40:
                neg = 1
                b &= 0xBF
            res |= b << j
            return (-res if neg else res), pos
        res |= (b & 0x7F) << j
        j += 7
    return 0, pos


def main():
    d = open(HDL, "rb").read()
    pos = 0
    page = 0
    found = {}
    while pos + 2 <= len(d):
        start = pos
        page_size = (d[pos] << 8) | d[pos + 1]
        pos += 2
        if page_size <= 2:
            break
        end = start + page_size
        if end > len(d):
            break
        last_h = 0
        last_off = 0
        idx = 0
        first_off = None
        while pos < end:
            old = pos
            dh, pos = umc(d, pos)
            do, pos = mc(d, pos)
            if pos == old:
                break
            last_h += dh
            last_off += do
            if idx == 0:
                first_off = last_off
            if last_h in TARGET:
                found[last_h] = (page, idx, last_off, first_off, last_off + ADDR_BASE)
            idx += 1
        page += 1
        pos = end + 2

    print("pages=%d" % page)
    for h in sorted(TARGET):
        if h in found:
            p, i, off, foff, addr = found[h]
            print("handle=%-7d page=%-3d idx=%-6d offset=%-10d page_first_off=%-10d addr=%d"
                  % (h, p, i, off, foff, addr))
        else:
            print("handle=%-7d 未在 Handles 页中命中" % h)


if __name__ == "__main__":
    sys.exit(main())
