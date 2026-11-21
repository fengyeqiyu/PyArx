# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_handle(b):
    hc=b.read_rc(); sz=hc&0xf; h=0
    for _ in range(sz): h=(h<<8)|b.read_rc()
    return hc,h
p=4
for n in range(6):
    b=BitStream(data); b.byte=p
    ms=b.read_ms()
    code=b.read_bb()
    if code==0: ot=b.read_rs()
    elif code==1: ot=b.read_rc()&0xff
    elif code==2: ot=0
    else: ot=256
    rl=b.read_rl()
    hc,h=read_handle(b)
    print(f"#{n} addr={p} ms={ms} type=0x{ot:X} rl={rl} h=0x{h:X} hc=0x{hc:X}")
    p+=ms+4
