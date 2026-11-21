# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc();size=code&0xF;v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
for start in [80393,80383,80377,80370]:
    b=BitStream(data); b.byte=start
    try:
        ms=b.read_ms();ot=b.read_bs();rl=b.read_rl();h=read_h(b)
        print(start,"ms",ms,"ot",hex(ot),"rl",rl,"h",hex(h),"end",start+ms+4)
    except Exception as e:
        print(start,"err",e)
