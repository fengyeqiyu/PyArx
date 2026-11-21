# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
def umc_enc(v):
    out=[]
    while True:
        b=v&0x7f; v>>=7
        if v: out.append(b|0x80)
        else: out.append(b); break
    return bytes(out)
for h in [0x4237a,0x423d5,0x423d9,0x42487,0x11cf7]:
    e=umc_enc(h)
    # also encode handleoff relative to 1
    e2=umc_enc(h-1)
    print(hex(h),"abs",e.hex(),"@",hdl.find(e),"rel-1",e2.hex(),"@",hdl.find(e2))
