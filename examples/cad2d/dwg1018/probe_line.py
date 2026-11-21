# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
exec(open(r"G:\PyArx\examples\cad2d\dwg1018\probe_objcrc.py").read().split("addr=4")[0])
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
addr=4; idx=0; lines=[]
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if end>=len(data): break
    if ot==0x13: lines.append((idx,addr,ms,rl,h,eed))
    idx+=1; addr=end
print("lines",len(lines))
i,a,ms,rl,h,eed=lines[0]
raw=data[a:a+ms]
print("LINE",i,"@",a,"ms",ms,"rl",rl,"h",hex(h),"eed",eed,"bodylen",len(raw))
print("body:", raw.hex(' '))
# 公共字段后起始位位置：MS(起始@a) + BS OT + RL bitsize + H + BS eed
b=BitStream(data); b.byte=a
ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
print("after common bitpos", b.tell(), "byte", b.byte, "bit", b.bit)
