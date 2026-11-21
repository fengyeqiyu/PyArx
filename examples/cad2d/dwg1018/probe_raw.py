# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=140531
# C: H@10.4, end=25; 数据在 H 之后。dump rel 10..27 原始字节
seg=data[addr+10:addr+28]
print("bytes rel10..:", seg.hex(' '))
# 逐字节
for i,x in enumerate(seg):
    print(f"  rel{10+i}: 0x{x:02X}")
