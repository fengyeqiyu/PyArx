# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()

def umc_enc(v):
    out=[]
    while v>=0x80:
        out.append((v&0x7F)|0x80); v>>=7
    out.append(v&0x7F); return bytes(out)

def mc_enc(v):
    neg=v<0; v=abs(v); out=[]
    while v>=0x80:
        out.append((v&0x7F)|0x80); v>>=7
    b=v&0x3F
    if neg: b|=0x40
    out.append(b); return bytes(out)

for label,v in [("handleoff 0x42379",0x42379),("handle 0x4237A",0x4237A),
                ("offset 2802",2802),("offset 2806",2806),("offset 156+4=160",160)]:
    u=umc_enc(v); m=mc_enc(v)
    hu=hdl.find(u); hm=hdl.find(m)
    print(f"{label}: UMC={u.hex()}@{hu}  MC={m.hex()}@{hm}")
