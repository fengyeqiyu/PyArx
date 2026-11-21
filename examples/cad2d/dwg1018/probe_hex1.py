# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return code,size,v

# first LINE addr
addr=4
b=BitStream(data); b.byte=addr
ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); c,s,h=read_h(b)
print(f"@addr={addr} ms={ms} ot={ot:#x} rl={rl} H code={c} size={s} h={h:#x}")
print("raw obj bytes:", data[addr:addr+ms].hex(' '))
# EED
while True:
    sz=b.read_bs()
    print(f"  eed bs={sz} pos {b.byte}.{b.bit}")
    if not sz: break
    c2,s2,h2=read_h(b); print(f"    handle code={c2} size={s2} h={h2:#x}")
    raw=b.read_tf(sz)
    print(f"    raw={raw.hex(' ') if raw else None} pos {b.byte}.{b.bit}")
print(f"after EED pos {b.byte}.{b.bit}")
print("next bytes:", data[b.byte:b.byte+16].hex(' '))
flag=b.read_rc(); print(f"flag={flag:#x} pos {b.byte}.{b.bit}")
objsize=b.read_rs(); print(f"ent.size={objsize} pos {b.byte}.{b.bit}")
