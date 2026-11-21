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

for L in lines[:3]:
    i,a,ms,rl,h=L
    b=BitStream(data); b.byte=a
    b.read_ms(); b.read_bs(); b.read_rl(); read_h(b); skip_eed(b)
    print(f"--- LINE {i} @{a} ms={ms} rl={rl} h={hex(h)} afterEED {b.byte}.{b.bit}")
    flag=b.read_rc()
    print(f"  flag_r11={flag:02X}")
    if flag:
        objsize=b.read_rs(); layer=read_h(b); print(f"  ent.size={objsize} layer={hex(layer)}")
    opts=b.read_rs(); print(f"  opts_r11={opts:04X}")
    if flag & 64: extra=b.read_rc()
    if flag & 1: color=b.read_rc(); print(f"  color={color}")
    if flag & 2: ltype=read_h(b); print(f"  ltype={hex(ltype)}")
    if flag & 4 and b.byte and 0x13 not in (0x13,): pass
    if flag & 8: thick=b.read_rd(); print(f"  thickness={thick}")
    if flag & 32: hh=read_h(b)
    prev=b.read_b(); print(f"  preview={prev}")
    entmode=b.read_bb(); nreact=b.read_bl()
    print(f"  entmode={entmode} nreact={nreact} pos {b.byte}.{b.bit}")
