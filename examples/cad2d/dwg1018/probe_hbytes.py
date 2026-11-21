# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
def umc_bytes(v):
    # encode like bit_write_UMC
    out=[]; j=0; tmp=v
    while True:
        b=tmp&0x7f; tmp>>=7
        if tmp: out.append(b|0x80)
        else:
            out.append(b); break
    return bytes(out)
for v in [1,2,3,5,0x4237a,0x42487,0x11CF7]:
    eb=umc_bytes(v)
    i=hdl.find(eb)
    print(hex(v),"umc",eb.hex(),"@",i)
