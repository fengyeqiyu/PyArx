# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def hcode(h):
    size=0
    v=h
    while v: size+=1; v>>=8
    code=size
    bs=h.to_bytes(size,'big') if size else b''
    return bytes([code])+bs
for h in [0x12,0x1b,0x20,0x22,0x23,0x11CF7,0x42487]:
    pat=hcode(h)
    i=data.find(pat)
    print(hex(h),"code",pat.hex(),"@",i)
