# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
# H: code byte size=code&0xF. handle 2 -> code=0x02, then val byte=0x02
# handle 3 -> 0x03 0x03; handle 4 ->0x04 0x04
for h in [2,3,4,5]:
    pat=bytes([h,h])
    hits=[i for i in range(0,4000) if data[i:i+2]==pat]
    print(f"handle {h} ({pat.hex()}) hits:", hits[:10])
