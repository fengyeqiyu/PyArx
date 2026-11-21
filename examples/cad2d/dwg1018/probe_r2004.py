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
def skip_eed(st):
    n=0
    while True:
        size=st.read_bs()
        if not size: break
        read_h(st); st.read_tf(size); n+=1
    return n

# locate first LINE
addr=4; idx=0; lines=[]
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if end>=len(data): break
    if ot==0x13: lines.append((idx,addr,ms,rl,h))
    idx+=1; addr=end

for L in lines[:2]:
    i,a,ms,rl,h=L
    b=BitStream(data); b.byte=a
    b.read_ms(); b.read_bs(); b.read_rl(); read_h(b); skip_eed(b)
    print(f"=== LINE {i} @{a} ms={ms} rl={rl} h={hex(h)} afterEED {b.byte}.{b.bit}")
    print("  raw:",data[a:a+ms+4].hex(' '))
    # Path A: ODA R2000+: B graphic flag
    p=BitStream(data); p.byte=a
    p.read_ms(); p.read_bs(); p.read_rl(); read_h(p); skip_eed(p)
    g=p.read_b(); print(f"  [A] graphic_present={g}")
    if g:
        gs=p.read_rl(); print(f"  [A] graphic size={gs}")
        p.read_tf(gs)
    nolinks=p.read_b(); print(f"  [A] nolinks={nolinks}")
    # ENC color BS
    enc=p.read_bs(); print(f"  [A] enc_color_bs={enc}")
    lts=p.read_bd(); print(f"  [A] ltype_scale={lts}")
    ltf=p.read_bb(); print(f"  [A] ltype_flags={ltf}")
    psf=p.read_bb(); print(f"  [A] plotstyle_flags={psf}")
    inv=p.read_bs(); print(f"  [A] invisible={inv}")
    lw=p.read_rc(); print(f"  [A] linewt={lw} pos {p.byte}.{p.bit}")
