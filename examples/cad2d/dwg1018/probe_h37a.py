# -*- coding: utf-8 -*-
obj=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
def find_all(buf,pat):
    idx=[];p=0
    while True:
        p=buf.find(pat,p)
        if p<0: break
        idx.append(p);p+=1
    return idx
for hx in ["42 37 a","42 3d 5","42 3d 9"]:
    pat=bytes(int(x,16) for x in hx.split())
    io=find_all(obj,pat)[:8]; ih=find_all(hdl,pat)[:8]
    print(hx,"obj",io,"hdl",ih)
