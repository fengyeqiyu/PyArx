# -*- coding: utf-8 -*-
"""定位 handle 段地址累加的 6 处偏差。"""
HDL = r"G:\PyArx\examples\cad2d\dwg1021\hdl_full.bin"
REF = r"G:\PyArx\examples\cad2d\dwg1021\arx_all1021.tsv"


def umc(b, i, n):
    r = 0
    j = 0
    for _ in range(8):
        c = b[i]; i += 1
        if not (c & 0x80):
            return r | (c << j), i
        r |= (c & 0x7F) << j; j += 7
    raise ValueError


def mc(b, i, n):
    r = 0
    j = 0
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
n = len(b)
i = 0
rows = []
page = 0
while i + 2 <= n:
    start = i
    size = (b[i] << 8) | b[i + 1]
    if size <= 2:
        break
    i += 2
    end = start + size
    last_h = last_o = 0
    k = 0
    while i < end:
        dh, i = umc(b, i, n)
        do, i = mc(b, i, n)
        last_h += dh
        last_o += do
        rows.append((page, k, dh, do, last_h, last_o + 2))
        k += 1
    i = end + 2
    page += 1

print("pages", page, "rows", len(rows))
print("page sizes sample:", [ (rows[j][0]) for j in range(0) ])

ref = []
with open(REF, encoding="latin-1") as f:
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) < 2:
            continue
        try:
            ref.append((int(p[0]), int(p[1])))
        except ValueError:
            continue

mis = [j for j in range(len(rows)) if rows[j][5] != ref[j][1]]
print("addr mismatches", len(mis), mis[:10])
for j in mis[:10]:
    pg, k, dh, do, h, a = rows[j]
    prev = rows[j - 1] if j else None
    print(f"#{j} page={pg} idx={k} dh={dh} do={do} h={h} calc={a} ref={ref[j][1]} "
          f"prev(page={prev[0]},idx={prev[1]},o={prev[5]})")

uniq_h = len(set(r[4] for r in rows))
print("unique computed handles", uniq_h)
print("first 8 handles", [r[4] for r in rows[:8]], "ref", [ref[j][0] for j in range(8)])
