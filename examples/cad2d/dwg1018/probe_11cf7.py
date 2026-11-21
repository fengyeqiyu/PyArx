# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return code,size,v
# scan backward from 19488853 for a plausible object header
for off in range(19488853-40, 19488853+1):
    b=BitStream(data); b.byte=off
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); code,size,h=read_h(b)
    except Exception: continue
    if 0<ms<0x4000 and 0<=ot<0x1f4 and h<0x100000:
        print(f"cand off={off} ms={ms} ot={hex(ot)} rl={rl} h={hex(h)} code={hex(code)} size={size}")
