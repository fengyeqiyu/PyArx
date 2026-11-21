# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return code,size,v
addr=4; n=0; line=None
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try: ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); c,s,h=read_h(b)
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    if ot==0x13 and line is None: line=(addr,ms,rl,h)
    n+=1; addr=addr+ms+4
print("first LINE",line)
a,ms,rl,h=line
print("raw:",data[a:a+ms].hex(' '))
b=BitStream(data); b.byte=a
ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); c,s,h2=read_h(b)
print(f"ms={ms} ot={ot:#x} rl={rl} H code={c} sz={s} h={h2:#x}")
while True:
    sz=b.read_bs(); print(f" eed bs={sz} pos {b.byte}.{b.bit}")
    if not sz: break
    c2,s2,h3=read_h(b); print(f"   h code={c2} sz={s2} {h3:#x}")
    raw=b.read_tf(sz); print(f"   raw={raw.hex(' ')} pos {b.byte}.{b.bit}")
print("after EED",b.byte,b.bit,"next:",data[b.byte:b.byte+12].hex(' '))
flag=b.read_rc(); print("flag",hex(flag),b.byte,b.bit)
objsize=b.read_rs(); print("ent.size",objsize)
cc,ss,layer=read_h(b); print("layer code",cc,"sz",ss,hex(layer))
opts=b.read_rs(); print("opts",hex(opts))
