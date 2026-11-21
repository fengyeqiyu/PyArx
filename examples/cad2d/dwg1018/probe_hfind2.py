# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
for h in [0x42487,0x11CF7,0x11CF5]:
    be=h.to_bytes(3,'big'); le=h.to_bytes(3,'little')
    ib=data.find(be); il=data.find(le)
    print(hex(h),"be",be.hex(),"@",ib,"le",le.hex(),"@",il)
# dump object @20367 full bytes with offset
a=20367
print("obj bytes:")
for i in range(a,a+70):
    print(f"{i-a:3d} {data[i]:02X}",end="  ")
    if (i-a)%8==7: print()
print()
