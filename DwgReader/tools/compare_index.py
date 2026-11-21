# -*- coding: utf-8 -*-
"""比对 C++ 索引 index_cpp.tsv 与 ARX 基准 arx_all1021.tsv。"""
import csv
import sys

CPP = r"G:\PyArx\DwgReader\build\index_cpp.tsv"
REF = r"G:\PyArx\examples\cad2d\dwg1021\arx_all1021.tsv"

ref = []
with open(REF, encoding="latin-1") as f:
    for row in csv.reader(f, delimiter="\t"):
        if len(row) < 2:
            continue
        try:
            handle = int(row[0])
            addr = int(row[1])
        except ValueError:
            continue  # BLOCK_HEADER 从属句柄续行
        typ = row[2] if len(row) > 2 else ""
        ref.append((handle, addr, typ))

cpp = []
with open(CPP, encoding="latin-1") as f:
    for row in csv.reader(f, delimiter="\t"):
        if len(row) < 3:
            continue
        cpp.append((int(row[0]), int(row[1]), int(row[2])))

print(f"cpp={len(cpp)} ref={len(ref)}")
n = min(len(cpp), len(ref))
h_mis = []
a_mis = []
t_mis = []
for i in range(n):
    ch, ca, ct = cpp[i]
    rh, ra, rt = ref[i]
    if ch != rh:
        h_mis.append((i, ch, rh))
    if ca != ra:
        a_mis.append((i, ca, ra))
    if rt.isdigit() and ct != int(rt):
        t_mis.append((i, ct, rt))

print(f"handle mismatch={len(h_mis)} addr mismatch={len(a_mis)} type mismatch={len(t_mis)}")
for label, mis in (("handle", h_mis), ("addr", a_mis), ("type", t_mis)):
    for x in mis[:10]:
        print(f"  {label} #{x[0]} cpp={x[1]} ref={x[2]}")
