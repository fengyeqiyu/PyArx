# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
def skip_eed(st):
    n=0
    while True:
        size=st.read_bs()
        if not size: break
        read_h(st); st.read_tf(size); n+=1
    return n

a=20367; b=BitStream(data); b.byte=a
ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
print(f"ms={ms} ot={hex(ot)} rl={rl} h={hex(h)} eedbs={eed} pos={b.byte}.{b.bit}")
ne=skip_eed(b)
print(f"after EED ({ne} items) pos={b.byte}.{b.bit} rel={b.byte-a}.{b.bit}")
# dump bits from afterEED
def dump_bits(st, nbits):
    out=[]
    for _ in range(nbits):
        out.append(st.read_b())
    return out
# Try spec order R13b1 block: preview B
st=BitStream(data); st.byte=a
# replay to after EED
st.read_ms(); st.read_bs(); st.read_rl(); read_h(st); skip_eed(st)
p0=st.tell()
print("preview B:",st.read_b(),"pos",st.byte,st.bit)
entmode=st.read_bb(); print("entmode BB:",entmode,"pos",st.byte,st.bit)
nreact=st.read_bl(); print("nreact BL:",nreact,"pos",st.byte,st.bit)
xdic=st.read_b(); print("is_xdic_missing B:",xdic,"pos",st.byte,st.bit)
# ENC color BS
flags=st.read_bs(); print("color flags BS:",flags,"pos",st.byte,st.bit)
