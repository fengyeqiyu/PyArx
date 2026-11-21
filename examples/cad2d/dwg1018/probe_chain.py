# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v

def try_obj(addr):
    bb=BitStream(data)
    try:
        bb.byte=addr; bb.bit=0
        ms=bb.read_ms(); ot=bb.read_bs(); rl=bb.read_rl(); h=read_h(bb); eed=bb.read_bs()
    except Exception:
        return None
    if not(0<=ot<0x1f4 and 0<h<0x100000 and 0<=eed<0x200 and 0<ms<0x40000 and rl>=0 and rl<ms*8+64):
        return None
    return ms,ot,rl,h,eed

addr=4
seq=[]
for k in range(6):
    r=try_obj(addr)
    if not r:
        print(f"#{k} @{addr} FAIL"); break
    ms,ot,rl,h,eed=r
    seq.append((addr,ms,ot,rl,h,eed))
    print(f"#{k} @{addr} MS={ms} OT={ot} RL={rl} H=0x{h:X} EED={eed}")
    addr=addr+ms+4
print("next addr", addr)
