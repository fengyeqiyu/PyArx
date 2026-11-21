# -*- coding: utf-8 -*-
auth = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_auth.bin", "rb").read()
off = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin", "rb").read()
print("auth", len(auth), "offline", len(off), "diff", len(off) - len(auth))

# 第一处差异
n = min(len(auth), len(off))
first = -1
for i in range(n):
    if auth[i] != off[i]:
        first = i
        break
print("first diff @", first)
if first >= 0:
    print("auth  :", " ".join("%02x" % b for b in auth[first-4:first+12]))
    print("offlin:", " ".join("%02x" % b for b in off[first-4:first+12]))

# 按页统计：每页内首个差异偏移
P = 0x7400
import collections
page_diff = {}
i = 0
while i < n:
    if auth[i] != off[i]:
        pg = i // P
        if pg not in page_diff:
            page_diff[pg] = i
    i += 1
print("pages with diff:", len(page_diff))
for pg, addr in sorted(page_diff.items())[:30]:
    print("  page", pg, "first diff @", addr, "page_start", pg*P)
