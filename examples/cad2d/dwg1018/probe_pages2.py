# -*- coding: utf-8 -*-
"""探针：按签名定位系统页，dump 页面头前后布局。"""
import struct
import sys

PM_SIG = 0x41630E3B
SM_SIG = 0x4163003B


def main(path):
    d = open(path, "rb").read()

    def find(sig):
        pat = struct.pack("<I", sig)
        out, i = [], 0
        while True:
            i = d.find(pat, i)
            if i < 0:
                break
            out.append(i)
            i += 1
        return out

    for sig, name in [(PM_SIG, "PageMap"), (SM_SIG, "SectionMap")]:
        for hit in find(sig):
            print(f"=== {name} sig @0x{hit:X} ===")
            lo = hit - 0x20
            for i in range(lo, hit + 0x30, 16):
                hx = " ".join(f"{b:02X}" for b in d[i:i + 16])
                asc = "".join(chr(b) if 32 <= b < 127 else "." for b in d[i:i + 16])
                print(f"{i:06X}  {hx:<47} {asc}")
            # 签名在 +0x20：尝试按 0x20 头布局解析
            base = hit - 0x20
            ptype, dsize, csize, ctype, cksum = struct.unpack_from("<LLLLL", d, base)
            print(f"  [base-0x20] type=0x{ptype:08X} decomp={dsize} compressed={csize} "
                  f"ctype={ctype} checksum=0x{cksum:08X}")
            # 签名后紧跟的字段
            f1, f2, f3, f4 = struct.unpack_from("<LLLL", d, hit + 4)
            print(f"  [after sig] {f1} {f2} {f3} {f4}")
            print()


if __name__ == "__main__":
    main(sys.argv[1])
