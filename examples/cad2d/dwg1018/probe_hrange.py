# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
addr=4; idx=0; hs=[]
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if end>=len(data): break
    hs.append(h); idx+=1; addr=end
print("count",len(hs),"min",hex(min(hs)),"max",hex(max(hs)))
print("first 20:",[hex(x) for x in hs[:20]])
print("last 10:",[hex(x) for x in hs[-10:]])
