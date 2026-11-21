# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
for h in [0x11CF7,0x11CF5,0x42487]:
    be=h.to_bytes(3,'big')
    idx=0; hits=[]
    while True:
        i=hdl.find(be,idx)
        if i<0: break
        hits.append(i); idx=i+1
    print(hex(h),"be",be.hex(),"hdl hits",hits[:10])
print("hdl len",len(hdl))
