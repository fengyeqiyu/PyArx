# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("section size",len(data))
# scan for sentinel 16-byte markers: DWG sentinels are fixed byte patterns; check tail
print("last 32 bytes:",data[-32:].hex(' '))
print("tail near 97795:",data[97780:97830].hex(' '))
# nonzero byte extent
nz=max(i for i,b in enumerate(data) if b)
print("last nonzero byte",nz)
# first RL 0xdca
import struct
print("first RL",hex(struct.unpack_from('<I',data,0)[0]))
# how much of the 22MB is zero
nonzero=sum(1 for b in data if b)
print("nonzero bytes",nonzero)
