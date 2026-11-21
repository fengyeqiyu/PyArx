# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
addr=4; n=0; res=[]
maxa=len(data)-4
while addr<maxa:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if end>=len(data): break
    res.append((addr,h,ot,ms))
    n+=1; addr=end
print("scanned",n,"end",addr)
for a,h,ot,ms in res[:25]:
    print(f"addr={a} h=0x{h:X} type=0x{ot:X} ms={ms}")
# 句柄增量序列
hs=[h for _,h,_,_ in res]
diffs=[hs[i]-hs[i-1] for i in range(1,len(hs))]
print("handle diffs first 25:",[hex(d) for d in diffs[:25]])
