# -*- coding: utf-8 -*-
py=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()[:11328]
cref=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl0_c_ref.bin","rb").read()
print("py len",len(py),"cref len",len(cref))
n=min(len(py),len(cref)); diff=-1
for i in range(n):
    if py[i]!=cref[i]: diff=i; break
print("first diff @",diff)
if diff>=0:
    print("py :",py[diff-8:diff+16].hex(' '))
    print("cref:",cref[diff-8:diff+16].hex(' '))
