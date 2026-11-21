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

i,a,ms,rl,h=lines[0]
b=BitStream(data); b.byte=a
b.read_ms(); b.read_bs(); b.read_rl(); read_h(b); skip_eed(b)
print("after EED:",b.byte,b.bit)
def pos(tag): print(f"  {tag} byte={b.byte} bit={b.bit}")
# common_entity_data (R2004):
flag_r11=b.read_rc(); pos("flag_r11"); print("  flag_r11",hex(flag_r11))
if flag_r11:
    size=b.read_rs(); pos("ent.size(common)"); layer=read_h(b); pos("layer h")
opts_r11=b.read_rs(); pos("opts_r11"); print("  opts_r11",hex(opts_r11))
# HAS_PSPACE bit16 -> extra_r11
if flag_r11 & 16:
    extra=b.read_rc(); pos("extra_r11")
# HAS_EED extra_r11 bit1 -> already done at common; skip
if flag_r11 & 1:
    color=b.read_rcd() if hasattr(b,'read_rcd') else b.read_rc(); pos("color")
# HAS_LTYPE bit2
if flag_r11 & 2:
    ltype=read_h(b); pos("ltype h")
# HAS_THICKNESS bit8
if flag_r11 & 8:
    th=b.read_rd(); pos("thickness")
# HAS_HANDLING bit32
if flag_r11 & 32:
    hh=read_h(b); pos("handling h")
# HAS_VIEWPORT extra bit2
# preview B
prev=b.read_b(); pos("preview"); print("  preview",prev)
# common_entity_handle_data
entmode=b.read_bb(); pos("entmode"); print("  entmode",entmode)
nreact=b.read_bl(); pos("num_reactors"); print("  nreact",nreact)
# reactors handle list
for _ in range(nreact):
    read_h(b); pos("reactor")
# xdicobj handle
xdic=b.read_b()
if xdic:
    read_h(b); pos("xdic h")
# ownerhandle
owner=read_h(b); pos("owner"); print("  owner",hex(owner))
# layer (R2000b)
layer=read_h(b); pos("layer"); print("  layer",hex(layer))
# ltype_flags BB
ltype_flags=b.read_bb(); pos("ltype_flags"); print("  ltype_flags",ltype_flags)
plotstyle=b.read_bb(); pos("plotstyle_flags")
# color BS
color=b.read_bs(); pos("color"); print("  color",color)
# ltype_scale BD1
ls=b.read_bd(); pos("ltype_scale"); print("  ltype_scale",ls)
# ltype_flags==3 ltype handle
if ltype_flags==3:
    lt=read_h(b); pos("ltype"); print("  ltype",hex(lt))
# invisible BS
inv=b.read_bs(); pos("invisible"); print("  invisible",inv)
# linewt RC
lw=b.read_rc(); pos("linewt"); print("  linewt",lw)
print("=== after common handle data:",b.byte,b.bit)
print("remaining body end", a+ms, "bytes left", a+ms-b.byte)
# LINE geometry
z=b.read_b(); pos("z_is_zero"); print("  z_is_zero",z)
sx=b.read_rd(); ex=b.read_dd(sx); sy=b.read_rd(); ey=b.read_dd(sy)
print("  start",sx,sy,"end",ex,ey)
pos("after geom")
