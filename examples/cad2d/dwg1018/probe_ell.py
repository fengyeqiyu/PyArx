# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
import entity

addr = int(sys.argv[1])
b = BitStream(entity.DATA); b.byte = addr
tc, bs, h = entity.read_header(b)
print("type=%x bitsize=%d handle=%d @%d.%d" % (tc, bs, h, b.byte, b.bit))
entity.decode_eed(b)
print("after eed @%d.%d" % (b.byte, b.bit))
entity.read_common(b)
print("after common @%d.%d" % (b.byte, b.bit))

def bd(tag):
    v = b.read_bd(); print("  BD %-10s = %r @%d.%d" % (tag, v, b.byte, b.bit)); return v

def be(tag):
    v = b.read_be(); print("  BE %-10s = %r @%d.%d" % (tag, v, b.byte, b.bit)); return v

cx, cy, cz = b.read_3bd(); print("center=%.3f,%.3f,%.3f @%d.%d" % (cx, cy, cz, b.byte, b.bit))
mx, my, mz = b.read_3bd(); print("sm_axis=%.3f,%.3f,%.3f @%d.%d" % (mx, my, mz, b.byte, b.bit))
be("extrusion(3BD)", ) if False else (lambda: print("  3BD extrusion=%r @%d.%d" % (b.read_3bd(), b.byte, b.bit)))()
bd("ratio"); bd("start"); bd("end")
