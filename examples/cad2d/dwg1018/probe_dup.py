# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc();size=code&0xF;v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
addr=4; seen={}; rows=[]
while addr<len(data)-10:
    b=BitStream(data);b.byte=addr
    try:
        ms=b.read_ms();ot=b.read_bs();rl=b.read_rl();h=read_h(b)
    except Exception: break
    if ms<=0 or ms>0x8000: break
    rows.append((addr,h,ot,ms)); seen[h]=addr
    addr=addr+ms+4
print("chain",len(rows),"end",addr)
for h in [1,2,3,5,0x12,0x1b]:
    print(hex(h),"chain@",seen.get(h))
print("first 6:",[(hex(a),hex(h),hex(t)) for a,h,t,_ in rows[:6]])
