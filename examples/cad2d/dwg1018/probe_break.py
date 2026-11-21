# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("around 97795:",data[97780:97830].hex(' '))
# last chain object @0x139ff size17408 end=97795
b=BitStream(data); b.byte=0x139ff
ms=b.read_ms();ot=b.read_bs();rl=b.read_rl()
code=b.read_rc();size=code&0xF;h=0
for _ in range(size): h=(h<<8)|b.read_rc()
print("lastobj ms",ms,"ot",hex(ot),"h",hex(h),"header end",b.byte)
print("next bytes @97795:",data[97795:97810].hex(' '))
