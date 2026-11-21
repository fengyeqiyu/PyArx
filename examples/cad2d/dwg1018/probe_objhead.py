# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("len",len(data))
for i in range(0,64,16):
    print(f"{i:6d}:",data[i:i+16].hex(' '))
