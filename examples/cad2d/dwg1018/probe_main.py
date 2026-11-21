# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=140531
for i in range(26,34):
    b=data[addr+i]
    print(i, f"{b:02X}", bin(b)[2:].zfill(8))
