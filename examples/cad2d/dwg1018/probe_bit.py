# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
body=140531-2
for i in range(8):
    b=data[body+i]
    print(i, f"{b:02X}", bin(b)[2:].zfill(8))
b=BitStream(data); b.byte=body
print("MS", b.read_ms(), f"@{b.byte-body}.{b.bit}")
print("BB", b.read_bb(), f"@{b.byte-body}.{b.bit}")
print("RC", b.read_rc(), f"@{b.byte-body}.{b.bit}")
