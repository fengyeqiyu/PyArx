# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return code,size,v

def walk(addr, depth=0):
    b=BitStream(data); b.byte=addr
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); c,s,h=read_h(b)
    eed=b.read_bs()
    print(f"{' '*depth}@{addr} ms={ms} ot={ot:#x} rl={rl} H code={c} sz={s} h={h:#x} eed={eed}")
    return ms,ot,rl,h,b

addr=4; n=0
while addr < len(data)-4 and n<12:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); c,s,h=read_h(b)
    except Exception as e:
        print("stop",e); break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000):
        print(f"@{addr} INVALID ms={ms} ot={ot:#x} h={h:#x}"); break
    print(f"obj{n} @{addr} ms={ms} ot={ot:#x} rl={rl} h={h:#x}")
    n+=1; addr=addr+ms+4
