# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(x):
    code=x.read_rc(); sz=code&0xf; v=0
    for _ in range(sz): v=(v<<8)|x.read_rc(); return code,sz,v

def decode_eed(b):
    while True:
        size=b.read_bs()
        if not size: break
        read_h(b); end=b.byte+size
        q=BitStream(data); q.byte=b.byte; q.bit=b.bit
        while q.byte < end:
            cc=q.read_rc()
            if cc==70: q.read_rs()
            elif cc==71: q.read_rl()
            elif cc in (3,5): q.read_rll_be()
            elif cc==1: q.read_rs()
            elif cc==2: q.read_rc()
            elif cc==0:
                l=q.read_rc(); q.read_tf(l)
            elif cc==4:
                l=q.read_rc(); q.read_tf(l)
            elif cc in (10,11,12,13,14,15):
                for _ in range(3): q.read_rd()
            elif cc in (40,41,42): q.read_rd()
            else: break
        b.byte=end

def decode(addr, exp):
    body=addr-2; b=BitStream(data); b.byte=body
    ms=b.read_ms(); ot=b.read_bs(); bitsize=b.read_rl()
    c=b.read_rc(); hsz=c&0xf
    for _ in range(hsz): b.read_rc()
    decode_eed(b)
    b.read_b()  # preview
    b.read_bb() # entmode
    b.read_bl() # numr
    b.read_b()  # xdic
    b.read_bs() # color
    b.read_bd() # ltscale
    b.read_bb() # lt_flags
    b.read_bb() # plotstyle
    b.read_bs() # inv
    b.read_rc() # linewt
    # hdl_dat 从 bitsize 位起，读 ownerhandle + layer
    h=BitStream(data); h.byte=body+(bitsize//8); h.bit=bitsize%8
    code,sz,owner=read_h(h)
    code2,sz2,layer=read_h(h)
    print(f"\naddr={addr} bitsize={bitsize} owner=0x{owner:X} layer=0x{layer:X} hdl@{h.byte-body}.{h.bit}")
    print(f"  dat after linewt rel={b.byte-addr}.{b.bit}")
    zz=b.read_b(); sx=b.read_rd(); ex=b.read_dd(sx); sy=b.read_rd(); ey=b.read_dd(sy)
    print(f"  zzero={zz} start=({sx},{sy}) end=({ex},{ey}) expect={exp}")

decode(140531,"(-300,200)-(-300,-200)")
decode(140595,"(-300,200)-(300,200)")
