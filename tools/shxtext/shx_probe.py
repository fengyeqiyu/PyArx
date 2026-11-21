# -*- coding: utf-8 -*-
"""探测 SHX 真实布局：unifont 的 ofs/count 与记录格式。"""
import struct
import sys

P = r"C:\Program Files\Autodesk\AutoCAD 2024\Fonts\romans.shx"


def u16(d, o):
    return struct.unpack_from("<H", d, o)[0]


def u32(d, o):
    return struct.unpack_from("<I", d, o)[0]


def main():
    d = open(P, "rb").read()
    sig = d[:40].split(b"\x1a")[0]
    print("size=%d sig=%r" % (len(d), sig))
    print("hex[20:40]:", d[20:40].hex(" "))
    for ofs in range(20, 34):
        print("  ofs=%2d u16=%5d u32=%d" % (ofs, u16(d, ofs), u32(d, ofs)))

    # 假设：ofs=26 u32 count，随后顺序 (u16 code, u16 len, data)
    for base in (26, 30, 25, 27):
        print("\n--- try base=%d ---" % base)
        cnt = u32(d, base)
        if not (10 < cnt < 2000):
            print("  count=%d 不合理" % cnt)
            continue
        p = base + 4
        recs = []
        ok = True
        for i in range(min(cnt, 8)):
            if p + 4 > len(d):
                ok = False
                break
            code, ln = u16(d, p), u16(d, p + 2)
            if ln > 2000 or p + 4 + ln > len(d):
                ok = False
                print("  断在 i=%d code=%d len=%d" % (i, code, ln))
                break
            recs.append((code, ln, d[p + 4:p + 4 + min(ln, 12)].hex(" ")))
            p += 4 + ln
        print("  count=%d  前 %d 条:" % (cnt, len(recs)))
        for r in recs:
            print("    code=0x%02X(%d) len=%d data=%s" % (r[0], r[0], r[1], r[2]))


if __name__ == "__main__":
    main()
