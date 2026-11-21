# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def hdr(addr):
    body=addr-2
    b=BitStream(data); b.byte=body
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl()
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return body,ms,ot,rl,v,b

for h,a in [(85,140531),(86,140595),(205,20638072)]:
    body,ms,ot,rl,v,b=hdr(a)
    print(f"h={h} addr={a} body={body} ms={ms} type=0x{ot:X} rl={rl} h=0x{v:X} match={v==h}")
    print("  bytes:",data[body:body+26].hex(' '))
