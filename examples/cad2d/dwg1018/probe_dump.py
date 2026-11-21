# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
addr=4; idx=0
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if ot==0x13:
        print(f"LINE idx={idx} addr={addr} ms={ms} end={end} rl={rl} h={hex(h)}")
        print("after common header:", f"{b.byte}.{b.bit}")
        # EED loop
        while True:
            size=b.read_bs()
            print(f"  eed size={size} @{b.byte}.{b.bit}")
            if not size: break
            hh=read_h(b); print(f"  eed handle={hex(hh)} after {b.byte}.{b.bit}")
            raw=b.read_tf(size); print(f"  eed raw={raw.hex(' ')} after {b.byte}.{b.bit}")
        print(f"after EED: {b.byte}.{b.bit}")
        flag=b.read_rc(); print(f"  flag={flag:02X} after {b.byte}.{b.bit}")
        if flag:
            objsize=b.read_rs(); layer=read_h(b); print(f"  ent.size={objsize} layer={hex(layer)} after {b.byte}.{b.bit}")
        opts=b.read_rs(); print(f"  opts={opts:04X} after {b.byte}.{b.bit}")
        break
    idx+=1; addr=end
