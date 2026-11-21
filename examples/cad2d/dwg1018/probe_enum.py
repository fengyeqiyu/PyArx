# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
# locate first LINE
addr=4; idx=0; target=None
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if ot==0x13: target=addr; break
    idx+=1; addr=end

a=target
b=BitStream(data); b.byte=a
b.read_ms(); b.read_bs(); b.read_rl(); read_h(b)
# EED
while True:
    sz=b.read_bs()
    if not sz: break
    read_h(b); b.read_tf(sz)
print(f"after EED {b.byte}.{b.bit}")
base=b.tell()
for off in range(0, 40):
    p=BitStream(data); p.byte=a; p.bit=0
    p.tell()
    # seek to base+off
    p.byte=(base+off)>>3; p.bit=(base+off)&7
    rs=p.read_rs(); bs=p.read_bs(); rc=p.read_rc()
    print(f"bit+{off:2d} @{p.byte}.{p.bit}: RS={rs:5d}({rs:04X}) BS={bs:4d} RC={rc:3d}")
