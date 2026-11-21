# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v

b=BitStream(data); b.byte=4
ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
print(f"obj0 @4 MS={ms} OT={ot} bitsize={rl} H=0x{h:X} EED={eed} byte={b.byte} bit={b.bit}")
print("next expected addr ~", 4+ms+4)

for cand in range(2800, 2814):
    bb=BitStream(data)
    try:
        m2=bb.read_ms(); t2=bb.read_bs(); r2=bb.read_rl(); h2=read_h(bb)
        if 0<=t2<0x1f4 and 0<h2<0x100000:
            print(f"  cand @{cand} MS={m2} OT={t2} RL={r2} H=0x{h2:X}")
    except Exception:
        pass
