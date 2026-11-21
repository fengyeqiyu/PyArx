# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
b=BitStream(data); b.byte=2806
ms=b.read_ms(); print("ms",ms,"@",b.byte,b.bit)
ot=b.read_bs(); print("type",hex(ot),"@",b.byte,b.bit)
rl=b.read_rl(); print("rl",rl,"@",b.byte,b.bit)
code=b.read_rc(); sz=code&0xf; print("hcode",hex(code),"sz",sz,"@",b.byte,b.bit)
u=0
for i in range(sz):
    x=b.read_rc(); u=(u<<8)|x; print("  rc",hex(x),"@",b.byte,b.bit)
print("H",hex(u))
