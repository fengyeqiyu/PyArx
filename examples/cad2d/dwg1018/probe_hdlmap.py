# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
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
            if b&0x40: neg=1;b&=0xbf
            return (-v if neg else v)
        v|=(b&0x7f)<<k;k+=7
    return 0
st=BitStream(hdl)
ss=st.read_rs_be(); print("page0 size",ss)
lh=0;lo=0
for i in range(12):
    ho=umc(st); off=mc(st); lh+=ho; lo+=off
    print(i,"handle",lh,"ho",ho,"loc",lo)
