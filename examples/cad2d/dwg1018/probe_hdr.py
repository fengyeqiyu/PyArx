# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return c,sz,v

addr=140531  # MS 之后的主流链头（=obj->address）
b=BitStream(data); b.byte=addr
print(f"start {b.byte-addr}.{b.bit}")
ot=b.read_bs(); print(f"type={ot} @{b.byte-addr}.{b.bit}")
bitsize=b.read_rl(); print(f"bitsize={bitsize} @{b.byte-addr}.{b.bit}")
c,sz,hv=read_h(b); print(f"H hcode=0x{c:X} h=0x{hv:X} @{b.byte-addr}.{b.bit}")
size=b.read_bs(); print(f"EED size={size} @{b.byte-addr}.{b.bit}")
