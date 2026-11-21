# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return c,sz,v

def decode_eed(b):
    size=b.read_bs()
    if not size: return False
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
    return True

def decode_line(addr):
    b=BitStream(data); b.byte=addr
    b.read_bs(); b.read_rl(); read_h(b)
    decode_eed(b)
    b.read_b()  # preview_exists
    b.read_bb()  # entmode
    b.read_bl()  # num_reactors
    b.read_b()   # xdic
    raw=b.read_bs()
    b.read_bd()  # ltscale
    b.read_bb()  # lt_flags
    b.read_bb()  # plotstyle
    b.read_bs()  # invisible
    b.read_rc()  # linewt
    zz=b.read_b()
    sx=b.read_rd(); ex=b.read_dd(sx); sy=b.read_rd(); ey=b.read_dd(sy)
    return sx,sy,ex,ey,raw&0x1ff

rows=[]
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt",encoding="utf-8"):
    p=line.rstrip("\n").split("\t")
    if len(p)>=8 and p[2]=="LINE":
        rows.append((int(p[1]), float(p[3]),float(p[4]),float(p[5]),float(p[6]), int(p[8])))

ok=bad=0; bads=[]
for addr,esx,esy,eex,eey,ecol in rows:
    try:
        sx,sy,ex,ey,col=decode_line(addr)
        good=(abs(sx-esx)<1e-6 and abs(sy-esy)<1e-6 and abs(ex-eex)<1e-6 and abs(ey-eey)<1e-6)
        if good: ok+=1
        else:
            bad+=1; bads.append((addr,(sx,sy,ex,ey),(esx,esy,eex,eey)))
    except Exception as e:
        bad+=1; bads.append((addr,"ERR",str(e)))
print(f"LINE total={len(rows)} ok={ok} bad={bad}")
for x in bads[:10]: print(" ", x)
