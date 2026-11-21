# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
# handle 0x42487: code RC, size=3 -> code=0x3, bytes 42 48 87
for pat in [bytes([0x43,0x42,0x48,0x87]), bytes([0x33,0x42,0x48,0x87]), bytes([0x42,0x48,0x87])]:
    idx=0; hits=[]
    while True:
        i=data.find(pat,idx)
        if i<0: break
        hits.append(i); idx=i+1
    print(pat.hex(),"hits",hits[:10])
