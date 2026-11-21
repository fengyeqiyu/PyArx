# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return c,sz,v

addr=140531
b=BitStream(data); b.byte=addr
ot=b.read_bs(); bitsize=b.read_rl()
c,sz,hv=read_h(b)
print(f"after H: {b.byte-addr}.{b.bit} h=0x{hv:X}")
size=b.read_bs()
print(f"EED size={size} @{b.byte-addr}.{b.bit}")
c,sz,hv=read_h(b)
print(f"after H: {b.byte-addr}.{b.bit} hcode=0x{c:X} h={hv}")
end=b.byte+size
q=BitStream(data); q.byte=b.byte; q.bit=b.bit
print(f"q start: {q.byte-addr}.{q.bit}  end={end-addr}")
for _ in range(3):
    code=q.read_rc()
    print(f"  code={code} @{q.byte-addr}.{q.bit}")
    if code==70: q.read_rs()
    elif code==71: q.read_rl()
    elif code in (3,5): q.read_rll_be()
    else: break
print(f"  q end: {q.byte-addr}.{q.bit}")
