# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
o=140529
for i in range(6,27):
    print(i, f"{data[o+i]:02X}", bin(data[o+i])[2:].zfill(8))
