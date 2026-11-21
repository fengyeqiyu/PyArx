# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
b=BitStream(data); b.byte=0
ss=b.read_rs_be(); print("sec_size",ss)
lh=0; lo=0
for i in range(10):
    old=b.byte
    ho=b.read_umc(); off=b.read_mc()
    lh+=ho; lo+=off
    print(i,"@",old,"ho",ho,"off",off,"h",lh,"loc",lo)
