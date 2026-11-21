# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
for i in range(0, 48):
    print(f"{i:3d} {data[i]:02X}", end="  ")
    if i%8==7: print()
print()
