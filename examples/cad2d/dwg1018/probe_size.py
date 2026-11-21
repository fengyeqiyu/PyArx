# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return c,sz,v

def decode_eed(b):
    size=b.read_bs()
    if not size: return 0
    read_h(b); end=b.byte+size
    while True:
        q=BitStream(data); q.byte=b.byte; q.bit=b.bit
        while q.byte < end:
            c=q.read_rc()
            if c==70: q.read_rs()
            elif c==71: q.read_rl()
            elif c in (3,5): q.read_rll_be()
            elif c==1: q.read_rs()
            elif c==2: q.read_rc()
            elif c==0:
                l=q.read_rc(); q.read_tf(l)
            elif c==4:
                l=q.read_rc(); q.read_tf(l)
            elif c in (10,11,12,13,14,15):
                for _ in range(3): q.read_rd()
            elif c in (40,41,42): q.read_rd()
            else: break
        b.byte=q.byte; b.bit=q.bit
        if b.byte < end - 1:
            size=end-b.byte+1; end=b.byte+size; continue
        break
    b.byte=end; b.bit=6
    return 1

def decode_line(addr):
    b=BitStream(data); b.byte=addr
    b.read_bs(); b.read_rl(); read_h(b)
    has=decode_eed(b)
    b.read_b(); b.read_bb(); b.read_bl(); b.read_b()
    raw=b.read_bs(); b.read_bd(); b.read_bb(); b.read_bb(); b.read_bs(); b.read_rc()
    zz=b.read_b(); sx=b.read_rd(); ex=b.read_dd(sx); sy=b.read_rd(); ey=b.read_dd(sy)
    return sx,sy,ex,ey,has

rows=[]
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt",encoding="utf-8"):
    p=line.rstrip("\n").split("\t")
    if len(p)>=8 and p[2]=="LINE":
        rows.append((int(p[1]), float(p[3]),float(p[4]),float(p[5]),float(p[6])))

from collections import Counter
size_ok=Counter(); size_bad=Counter()
for addr,esx,esy,eex,eey in rows:
    body=addr-2; bb=BitStream(data); bb.byte=body; ms=bb.read_ms()
    try:
        sx,sy,ex,ey,has=decode_line(addr)
        good=abs(sx-esx)<1e-6 and abs(sy-esy)<1e-6 and abs(ex-eex)<1e-6 and abs(ey-eey)<1e-6
        (size_ok if good else size_bad)[(ms,has)]+=1
    except Exception as e:
        size_bad[(ms,"ERR")]+=1
print("OK (size,has_eed):", dict(size_ok.most_common(10)))
print("BAD (size,has_eed):", dict(size_bad.most_common(10)))
