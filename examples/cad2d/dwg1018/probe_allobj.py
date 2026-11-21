# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v

# 逐字节扫描，尝试 MS->BS(OT)->RL->H
valid=[]
for addr in range(4, 4000):
    bb=BitStream(data)
    try:
        bb.byte=addr; bb.bit=0
        ms=bb.read_ms(); ot=bb.read_bs(); rl=bb.read_rl(); h=read_h(bb)
    except Exception:
        continue
    if 1<=ms<0x2000 and 1<=ot<0x1f4 and 0<=rl<ms*8+32 and 1<=h<0x10000:
        valid.append((addr,ms,ot,rl,h))
for v in valid[:30]:
    print(f"@{v[0]} MS={v[1]} OT={v[2]} RL={v[3]} H=0x{v[4]:X}")
print("total", len(valid))
