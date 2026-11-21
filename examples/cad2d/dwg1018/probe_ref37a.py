# -*- coding: utf-8 -*-
obj=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
for hx in ["42 37 a","42 3d 5","42 3d 9","42 3d a"]:
    pat=bytes(int(x,16) for x in hx.split())
    idx=[];p=0
    while True:
        p=obj.find(pat,p)
        if p<0: break
        idx.append(p);p+=1
    print(hx, idx[:10])
