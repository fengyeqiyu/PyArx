# -*- coding: utf-8 -*-
py=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()[:26816]
cref=open(r"G:\PyArx\examples\cad2d\dwg1018\obj0_c_ref.bin","rb").read()
print("py",len(py),"cref",len(cref))
n=min(len(py),len(cref)); diff=-1
for i in range(n):
    if py[i]!=cref[i]: diff=i; break
print("first diff @",diff)
if diff>=0:
    print("py :",py[diff-8:diff+16].hex(' '))
    print("cref:",cref[diff-8:diff+16].hex(' '))
