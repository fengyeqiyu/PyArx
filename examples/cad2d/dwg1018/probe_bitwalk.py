# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
b=BitStream(data); b.byte=4
print("start",b.byte,b.bit)
ms=b.read_ms(); print("MS",ms,"@",b.byte,b.bit)
code=b.read_bb(); print("BB code",code,"@",b.byte,b.bit)
if code==0: ot=b.read_rs()
elif code==1: ot=b.read_rc()&0xff
elif code==2: ot=0
else: ot=256
print("BS type",hex(ot),"@",b.byte,b.bit)
rl=b.read_rl(); print("RL bitsize",rl,"@",b.byte,b.bit)
hcode=b.read_rc(); hsz=hcode&0xf; h=0
for _ in range(hsz): h=(h<<8)|b.read_rc()
print("H",hex(h),"hcode",hex(hcode),"@",b.byte,b.bit)
