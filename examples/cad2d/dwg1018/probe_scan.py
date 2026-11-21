# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
# walk chain from 4, record addr/handle/type/size
addr=4; objs=[]
while addr < len(data)-10:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    except Exception: break
    if ms<=0 or ms>0x8000: break
    objs.append((addr,h,ot,ms))
    addr=addr+ms+4
print("chain count",len(objs),"end",addr)
print("first",[(hex(a),hex(h),hex(t),m) for a,h,t,m in objs[:5]])
print("last",[(hex(a),hex(h),hex(t),m) for a,h,t,m in objs[-5:]])
# handles in chain
chainh={h for _,h,_,_ in objs}
print("chain handle max",hex(max(chainh)))
