# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
stream=BitStream(data)
for pg in range(11):
    start=stream.byte
    sec=stream.read_rs_be()
    end=start+sec
    n=0
    while stream.byte<end:
        ho=stream.read_umc(); off=stream.read_mc(); n+=1
    er=stream.read_rs_be()
    print(pg,"start",start,"sec",sec,"end",stream.byte,"entries",n)
# 页10 首条原始字节
print("around 20350:", data[20346:20360].hex(' '))
