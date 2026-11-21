# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return code,size,v
a=20367; b=BitStream(data); b.byte=a
ms=b.read_ms(); print("ms",ms,"pos",b.byte,b.bit)
ot=b.read_bs(); print("ot",hex(ot),"pos",b.byte,b.bit)
rl=b.read_rl(); print("rl",rl,"pos",b.byte,b.bit)
code,size,h=read_h(b); print("H code",hex(code),"size",size,"h",hex(h),"pos",b.byte,b.bit)
eed=b.read_bs(); print("eedbs",eed,"pos",b.byte,b.bit)
# raw first bytes
print("raw:",data[a:a+20].hex(' '))
