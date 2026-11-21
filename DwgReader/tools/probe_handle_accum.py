# -*- coding: utf-8 -*-
"""用 ARX 基准反推 AcDb:Handles 段 (handle, offset) 累加口径。

页结构：RS(size,BE) + N 条 [UMC(handle 增量) + MC(offset 增量)] + RS(crc,BE)
"""
HDL = r"G:\PyArx\examples\cad2d\dwg1021\hdl_full.bin"
REF = r"G:\PyArx\examples\cad2d\dwg1021\arx_all1021.tsv"


def umc(b, i, n):
    result = 0
    j = 0
    for _ in range(8):
        c = b[i]
        i += 1
        if not (c & 0x80):
            result |= c << j
            return result, i
        result |= (c & 0x7F) << j
        j += 7
    raise ValueError("umc overflow")


def mc(b, i, n):
    result = 0
    j = 0
    for _ in range(5):
        c = b[i]
        i += 1
        if not (c & 0x80):
            neg = 1 if (c & 0x40) else 0
            if neg:
                c &= 0xBF
            result |= c << j
            return (-result if neg else result), i
        result |= (c & 0x7F) << j
        j += 7
    raise ValueError("mc overflow")


def parse(handle_reset=False, offset_reset=False, base=0):
    b = open(HDL, "rb").read()
    n = len(b)
    i = 0
    hs, os_ = [], []
    last_h = last_o = 0
    pages = 0
    while i + 2 <= n:
        start = i
        size = (b[i] << 8) | b[i + 1]
        if size <= 2:
            break
        i += 2
        end = start + size
        if handle_reset:
            last_h = 0
        if offset_reset:
            last_o = 0
        while i < end:
            dh, i = umc(b, i, n)
            do, i = mc(b, i, n)
            last_h += dh
            last_o += do
            hs.append(last_h)
            os_.append(last_o + base)
        # 页尾 CRC（2 字节 BE）
        i = end + 2
        pages += 1
    return hs, os_, pages


ref = []
with open(REF, encoding="latin-1") as f:
    for line in f:
        parts = line.rstrip("\n").split("\t")
        if len(parts) < 2:
            continue
        try:
            h = int(parts[0])
            a = int(parts[1])
        except ValueError:
            continue
        ref.append((h, a))
print("ref entries =", len(ref))

for hr in (False, True):
    for orst in (False, True):
        for base in (0, 2):
            hs, os_, pages = parse(hr, orst, base)
            k = min(len(hs), len(ref))
            hm = sum(1 for j in range(k) if hs[j] != ref[j][0])
            am = sum(1 for j in range(k) if os_[j] != ref[j][1])
            print(f"handle_reset={hr} off_reset={orst} base={base}: pages={pages} n={len(hs)} "
                  f"handle_mis={hm} addr_mis={am} (cmp {k})")
