# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

# 权威 base=140531, sx@30.7 期望 u=0xC072C00000000000
base=140531
b=BitStream(data); b.byte=base+30; b.bit=7
u=b.read_rll()
print(f"py sx u=0x{u:016X} after={b.byte}.{b.bit}")
print("expect 0xC072C00000000000 after 38.7")

b2=BitStream(data); b2.byte=base+39; b2.bit=1
u2=b2.read_rll()
print(f"py sy u=0x{u2:016X} after={b2.byte}.{b2.bit} d={struct.unpack('<d',struct.pack('<Q',u2))[0]}")
print("expect 0x4069000000000000 d=200 after 47.1")
