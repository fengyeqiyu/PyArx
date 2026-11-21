# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc();size=code&0xF;v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return code,size,v
for addr in [4,2890,7614,43053]:
    b=BitStream(data);b.byte=addr
    try:
        ms=b.read_ms();ot=b.read_bs();rl=b.read_rl();code,sz,h=read_h(b)
        print(f"addr={addr} ms={ms} ot={hex(ot)} rl={rl} h={hex(h)} hdrend={b.byte}")
    except Exception as e:
        print(addr,"err",e)
print("bytes@2890:",data[2880:2900].hex(' '))
