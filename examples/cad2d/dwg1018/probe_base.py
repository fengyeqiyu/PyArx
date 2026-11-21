# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("first 12 bytes:",data[:12].hex(' '))
for base in (4,6):
    b=BitStream(data); b.byte=base
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl()
    c=b.read_rc(); sz=c&0xf; h=0
    for _ in range(sz): h=(h<<8)|b.read_rc()
    print(f"base={base} ms={ms} type=0x{ot:X} rl={rl} h=0x{h:X}")
