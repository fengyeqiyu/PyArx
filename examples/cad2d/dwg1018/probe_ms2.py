# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
b=BitStream(data); b.byte=4
w1=b.read_rs(); print("RS w1=",w1, "0x%X"%w1, "byte",b.byte)
b2=BitStream(data); b2.byte=4
m=b2.read_ms(); print("MS=",m,"byte",b2.byte,"bit",b2.bit)
# 2798 编码 RS: 2798<0x8000 -> 单字 LE: lo=2798&FF, hi=2798>>8
print("2798 LE bytes:", (2798&0xFF), (2798>>8))
print("2802 LE bytes:", (2802&0xFF), (2802>>8))
