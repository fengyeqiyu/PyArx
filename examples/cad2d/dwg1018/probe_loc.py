# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def hcode(h):
    v=h;size=0
    while v:size+=1;v>>=8
    return bytes([size])+h.to_bytes(size,'big')
true={}
for h in [0x12,0x1b,0x20,0x22,0x23]:
    true[h]=data.find(hcode(h))
print("true loc:",{hex(k):v for k,v in true.items()})

hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
def umc(st):
    v=0;k=0
    for _ in range(8):
        b=st.read_rc()
        if not b&0x80: return v|(b<<k)
        v|=(b&0x7f)<<k;k+=7
    return 0
def mc(st):
    neg=0;v=0;k=0
    for _ in range(5):
        b=st.read_rc()
        if not b&0x80:
            if b&0x40:neg=1;b&=0xbf
            return (-v if neg else v)
        v|=(b&0x7f)<<k;k+=7
    return 0
st=BitStream(hdl); st.read_rs_be()
lh=0;lo=0
want={0x12,0x1b,0x20,0x22,0x23}
got={}
for i in range(1500):
    ho=umc(st);off=mc(st);lh+=ho;lo+=off
    if lh in want: got[lh]=lo
print("mc loc:",{hex(k):v for k,v in got.items()})
