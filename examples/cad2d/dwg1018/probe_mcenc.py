# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
def mc_enc(v):
    neg=v<0; v=abs(v)
    bytes5=[0]*5
    j=0
    for i in range(5):
        bytes5[i]=(v>>j)&0x7f; j+=7
    # trim leading zero
    top=4
    while top>0 and not bytes5[top]: top-=1
    if bytes5[top]&0x40 and top>0: top-=1
    out=bytes5[:top+1]
    for i in range(top): out[i]|=0x80
    if neg: out[top]|=0x40
    return bytes(reversed(out))
for loc in [363755,134618,443689,423950,137948,2806,2966,4]:
    e=mc_enc(loc)
    i=hdl.find(e)
    print(loc,"enc",e.hex(),"@",i)
