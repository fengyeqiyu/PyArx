# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    code=b.read_rc(); sz=code&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return code,sz,v
def decode_eed(b):
    while True:
        size=b.read_bs()
        if not size: break
        code,hsz,hv=read_h(b); end=b.tell()+size*8
        while b.tell()<end:
            c=b.read_rc()
            if c==70: b.read_rs()
            elif c==71: b.read_rl()
            elif c in (3,5): b.read_rll_be()
            elif c==1: b.read_rs()
            elif c==2: b.read_rc()
            elif c==0:
                l=b.read_rc(); b.read_tf(l)
            elif c==4:
                l=b.read_rc(); b.read_tf(l)
            elif c in (10,11,12,13,14,15):
                for _ in range(3): b.read_rd()
            elif c in (40,41,42): b.read_rd()
            else: break

addr=140531; body=addr-2
b=BitStream(data); b.byte=body
b.read_ms(); b.read_bs(); b.read_rl()
c=b.read_rc(); hsz=c&0xf
for _ in range(hsz): b.read_rc()
decode_eed(b)
b.read_b(); b.read_bb(); b.read_bl(); b.read_b()
b.read_bs(); b.read_bd(); b.read_bb(); b.read_bb(); b.read_bs(); b.read_rc()
zz=b.read_b()
base=addr  # reset 后 chain 前移到 address
print(f"after zzero abs={b.byte}.{b.bit} relbyte={b.byte-base}")
seg=data[body:body+64]
print("seg:",seg.hex(' '))
# 权威: sx@30.7 sy@39.1 ex@47.3
for (rb,bit,name) in [(30,7,"sx"),(39,1,"sy"),(47,3,"ex")]:
    q=BitStream(data); q.byte=base+rb; q.bit=bit
    v=q.read_rd()
    print(f"  {name} @{rb}.{bit} = {v}")
