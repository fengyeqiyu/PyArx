# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=140531; body=addr-2
b=BitStream(data); b.byte=body
b.read_ms(); b.read_bs(); b.read_rl()
c=b.read_rc(); hsz=c&0xf
for _ in range(hsz): b.read_rc()
print(f"after header rel={b.byte-body}.{b.bit}")
def read_h(x):
    code=x.read_rc(); sz=code&0xf; v=0
    for _ in range(sz): v=(v<<8)|x.read_rc(); return code,sz,v
while True:
    size=b.read_bs()
    if not size: break
    read_h(b); end=b.tell()+size*8
    while b.tell()<end:
        cc=b.read_rc()
        if cc==70: b.read_rs()
        elif cc==71: b.read_rl()
        elif cc in (3,5): b.read_rll_be()
        elif cc==1: b.read_rs()
        elif cc==2: b.read_rc()
        elif cc==0:
            l=b.read_rc(); b.read_tf(l)
        elif cc==4:
            l=b.read_rc(); b.read_tf(l)
        elif cc in (10,11,12,13,14,15):
            for _ in range(3): b.read_rd()
        elif cc in (40,41,42): b.read_rd()
        else: break
print(f"after EED rel={b.byte-body}.{b.bit}")
seg=data[body+7:body+14]
print("bytes:",seg.hex(' '))
b2=BitStream(data); b2.byte=body+7; b2.bit=5
for i in range(40):
    print(f"  bit @{b2.byte-body}.{b2.bit} = {b2.read_b()}")
