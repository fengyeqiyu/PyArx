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

def decode_line(addr):
    body=addr-2; b=BitStream(data); b.byte=body
    ms=b.read_ms(); ot=b.read_bs(); bitsize=b.read_rl()
    c=b.read_rc(); hsz=c&0xf
    for _ in range(hsz): b.read_rc()
    decode_eed(b)
    b.read_b(); b.read_bb(); b.read_bl(); b.read_b()
    b.read_bs(); b.read_bd(); b.read_bb(); b.read_bb(); b.read_bs(); b.read_rc()
    h=BitStream(data); h.byte=body+(bitsize//8); h.bit=bitsize%8
    read_h(h); read_h(h)
    zz=b.read_b(); sx=b.read_rd(); ex=b.read_dd(sx); sy=b.read_rd(); ey=b.read_dd(sy)
    return sx,sy,ex,ey

# 读 ref_map LINE 行
ref={}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt","r",encoding="utf-8"):
    p=line.rstrip("\n").split("\t")
    if len(p)>=8 and p[2]=="LINE":
        ref[int(p[1])]=(float(p[3]),float(p[4]),float(p[5]),float(p[6]))

keys=list(ref.items())
ok=0; bad=[]
for addr,(ax,ay,bx,by) in keys:
    try:
        sx,sy,ex,ey=decode_line(addr)
        if abs(sx-ax)<1e-4 and abs(sy-ay)<1e-4 and abs(ex-bx)<1e-4 and abs(ey-by)<1e-4:
            ok+=1
        else:
            bad.append((addr,(ax,ay,bx,by),(sx,sy,ex,ey)))
    except Exception as e:
        bad.append((addr,"ERR",str(e)))
print(f"LINE 校验 {ok}/{len(keys)} 匹配")
for b in bad[:10]: print("  BAD",b)
