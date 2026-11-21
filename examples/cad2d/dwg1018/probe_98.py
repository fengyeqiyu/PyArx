# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=20374376
b=BitStream(data); b.byte=addr-2
ms=b.read_ms(); print("ms",ms)
b2=BitStream(data); b2.byte=addr
ot=b2.read_bs(); bitsize=b2.read_rl()
def read_h(x):
    c=x.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|x.read_rc()
    return c,sz,v
c,sz,hv=read_h(b2)
print(f"h=0x{hv:X} type={ot} bitsize={bitsize} afterH {b2.byte-addr}.{b2.bit}")
size=b2.read_bs(); print(f"EED size={size} @{b2.byte-addr}.{b2.bit}")
read_h(b2); end=b2.byte+size
print(f"end={end-addr}")
q=BitStream(data); q.byte=b2.byte; q.bit=b2.bit
while q.byte<end:
    cc=q.read_rc(); print(f"  code={cc} @{q.byte-addr}.{q.bit}")
    if cc==70: q.read_rs()
    elif cc==71: q.read_rl()
    elif cc in (3,5): q.read_rll_be()
    elif cc==1: q.read_rs()
    elif cc==2: q.read_rc()
    elif cc==0:
        l=q.read_rc(); print("   str len",l); q.read_tf(l)
    elif cc==4:
        l=q.read_rc(); q.read_tf(l)
    elif cc in (10,11,12,13,14,15):
        for _ in range(3): q.read_rd()
    elif cc in (40,41,42): q.read_rd()
    else: print("   ??"); break
print(f"q end {q.byte-addr}.{q.bit}")
