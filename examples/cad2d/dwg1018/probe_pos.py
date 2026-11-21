# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=140531; body=addr-2
b=BitStream(data); b.byte=body
b.read_ms(); b.read_bs(); b.read_rl()
c=b.read_rc(); hsz=c&0xf
for _ in range(hsz): b.read_rc()
def read_h(x):
    code=x.read_rc(); sz=code&0xf; v=0
    for _ in range(sz): v=(v<<8)|x.read_rc(); return code,sz,v
while True:
    size=b.read_bs()
    if not size: break
    read_h(b); end=b.byte+size
    sav=b.byte
    # 数据在副本位置解码（不推进主流）
    q=BitStream(data); q.byte=sav; q.bit=0
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
    b.byte=end; b.bit=0
def rel(): return f"{b.byte-addr}.{b.bit}"
print("after EED rel(addr)=",rel())
entmode=b.read_bb(); print("entmode",entmode,"@",rel())
numr=b.read_bl(); print("numr",numr,"@",rel())
xdic=b.read_b(); print("xdic",xdic,"@",rel())
raw=b.read_bs(); print("color",raw,"@",rel())
lt=b.read_bd(); print("ltscale",lt,"@",rel())
lf=b.read_bb(); print("lt_flags",lf,"@",rel())
ps=b.read_bb(); print("plotstyle",ps,"@",rel())
inv=b.read_bs(); print("inv",inv,"@",rel())
lw=b.read_rc(); print("linewt",lw,"@",rel())
print("C: entmode7.5 numr7.7 xdic8.1 color8.2 ltscale8.4 lt_flags8.6 plotstyle9.0 inv9.2 linewt9.4")
