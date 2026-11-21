# -*- coding: utf-8 -*-
py=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
c1=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl1_c.bin","rb").read()
base=29696
n=min(len(c1),16192)
diff=[i for i in range(n) if py[base+i]!=c1[i]]
print("page1 diff count",len(diff),"first",diff[:5])
if diff:
    d=diff[0]; print("py",py[base+d-4:base+d+12].hex(' ')); print("c ",c1[d-4:d+12].hex(' '))
