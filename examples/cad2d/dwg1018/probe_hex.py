# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin", "rb").read()
addr = int(sys.argv[1]) if len(sys.argv) > 1 else 14617392
n = int(sys.argv[2]) if len(sys.argv) > 2 else 12
for i in range(0, n):
    print("%3d: %02x" % (i, data[addr + i]), end="  ")
    if i % 8 == 7:
        print()
print()
b = BitStream(data); b.byte = addr
ot = b.read_bs(); print("type BS =", ot, "@", b.byte - addr, ".", b.bit)
# bitsize RL 分解
w1 = b.read_rs(); p1 = (b.byte - addr, b.bit)
w2 = b.read_rs(); p2 = (b.byte - addr, b.bit)
print("bitsize RS w1=0x%04x @%d.%d  w2=0x%04x @%d.%d  RL=0x%X" % (w1, p1[0], p1[1], w2, p2[0], p2[1], (w2 << 16) | w1))
c = b.read_rc(); print("handle code RC=0x%02x @%d.%d  size=%d code_hi=%d" % (c, b.byte - addr, b.bit, c & 0xf, (c >> 4) & 0xf))
sz = c & 0xf; v = 0
for _ in range(sz):
    v = (v << 8) | b.read_rc()
print("handle v=0x%X @%d.%d" % (v, b.byte - addr, b.bit))
print("EED size BS =", b.read_bs(), "@", b.byte - addr, ".", b.bit)
