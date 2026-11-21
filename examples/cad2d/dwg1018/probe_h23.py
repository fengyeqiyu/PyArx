# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def hcode(h):
    v=h;s=0
    while v:s+=1;v>>=8
    return bytes([s])+h.to_bytes(s,'big')
for h in [1,2,3,5,12,0x1b,0x20]:
    p=hcode(h); i=data.find(p)
    print(hex(h),"code",p.hex(),"first@",i)
