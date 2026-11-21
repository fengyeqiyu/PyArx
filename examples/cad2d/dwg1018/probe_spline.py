# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
import entity

addr = 3135706
b = BitStream(entity.DATA); b.byte = addr
tc, bs, h = entity.read_header(b)
entity.decode_eed(b)
entity.read_common(b)

scenario = b.read_bl(); degree = b.read_bl()
rational = b.read_b(); closed = b.read_b(); periodic = b.read_b()
b.read_bd(); b.read_bd()
n_knots = b.read_bl(); n_ctrl = b.read_bl()
weighted = b.read_b()
n_fit = b.read_bs()
knots = [b.read_bd() for _ in range(n_knots)]
for _ in range(n_ctrl):
    b.read_3bd()
    if weighted: b.read_bd()
print("nfit", n_fit, "nk", n_knots, "nc", n_ctrl, "wt", weighted, "knots", knots)
print("pos byte,bit", b.byte, b.bit)
# peek 后续原始字节
print("next bytes:", [hex(x) for x in entity.DATA[b.byte:b.byte+16]])
# 试读 BL
c = BitStream(entity.DATA); c.byte=b.byte; c.bit=b.bit
print("BL =", c.read_bl())
c2 = BitStream(entity.DATA); c2.byte=b.byte; c2.bit=b.bit
print("BS =", c2.read_bs())
c3 = BitStream(entity.DATA); c3.byte=b.byte; c3.bit=b.bit
print("RC =", c3.read_rc())
c4 = BitStream(entity.DATA); c4.byte=b.byte; c4.bit=b.bit
print("BB =", c4.read_bb())
