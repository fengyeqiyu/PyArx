# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=140531
b=BitStream(data); b.byte=addr
b.read_bs(); b.read_rl()
def read_h(b):
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return c,sz,v
read_h(b)
size=b.read_bs(); read_h(b); end=b.byte+size
q=BitStream(data); q.byte=b.byte; q.bit=b.bit
print(f"q start {q.byte-addr}.{q.bit} end={end-addr}")
c=q.read_rc(); print(f"code={c} @{q.byte-addr}.{q.bit}"); v=q.read_rs(); print(f"  rs={v} @{q.byte-addr}.{q.bit}")
c=q.read_rc(); print(f"code={c} @{q.byte-addr}.{q.bit}"); v=q.read_rl(); print(f"  rl={v} @{q.byte-addr}.{q.bit}")
c=q.read_rc(); print(f"code={c} @{q.byte-addr}.{q.bit}"); v=q.read_rll_be(); print(f"  rllbe={v:#x} @{q.byte-addr}.{q.bit}")
