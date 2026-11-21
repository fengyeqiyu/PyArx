# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return c,sz,v

addr=20638072
b=BitStream(data); b.byte=addr
ot=b.read_bs(); bitsize=b.read_rl()
c,sz,hv=read_h(b)
print(f"h=0x{hv:X} type=0x{ot:X} bitsize={bitsize} after H {b.byte-addr}.{b.bit}")
# EED size
size=b.read_bs(); print(f"EED size={size} @{b.byte-addr}.{b.bit}")
entmode=b.read_bb(); print(f"entmode={entmode} @{b.byte-addr}.{b.bit}")
numr=b.read_bl(); print(f"numr={numr} @{b.byte-addr}.{b.bit}")
xdic=b.read_b(); print(f"xdic={xdic} @{b.byte-addr}.{b.bit}")
raw=b.read_bs(); print(f"color=0x{raw:X} @{b.byte-addr}.{b.bit}")
lt=b.read_bd(); print(f"ltscale={lt} @{b.byte-addr}.{b.bit}")
lf=b.read_bb(); print(f"lt_flags={lf} @{b.byte-addr}.{b.bit}")
ps=b.read_bb(); print(f"plotstyle={ps} @{b.byte-addr}.{b.bit}")
inv=b.read_bs(); print(f"inv={inv} @{b.byte-addr}.{b.bit}")
lw=b.read_rc(); print(f"linewt=0x{lw:X} @{b.byte-addr}.{b.bit}")
zz=b.read_b(); sx=b.read_rd(); ex=b.read_dd(sx); sy=b.read_rd(); ey=b.read_dd(sy)
print(f"zz={zz} start=({sx},{sy}) end=({ex},{ey}) expect=(0.75,0)-(0.75,0.6)")
