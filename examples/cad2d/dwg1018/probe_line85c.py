# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    code=b.read_rc(); sz=code&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return code,sz,v

def read_eed(b, objsize):
    idx=0
    while True:
        size=b.read_bs()
        if not size: break
        code,sz,hv=read_h(b)
        raw=b.read_tf(size)
        print(f"  eed{idx} size={size} hcode={code} hsz={sz} h=0x{hv:X} raw={raw[:12].hex(' ')}")
        idx+=1
        if idx>50: print("  eed too many"); break
    return idx

for h,a in [(85,140531),(86,140595),(205,20638072)]:
    body=a-2
    b=BitStream(data); b.byte=body
    ms=b.read_ms(); ot=b.read_bs(); bitsize=b.read_rl()
    c=b.read_rc(); hsz=c&0xf; hv=0
    for _ in range(hsz): hv=(hv<<8)|b.read_rc()
    print(f"\nh={h} body={body} ms={ms} type=0x{ot:X} bitsize={bitsize} h=0x{hv:X} @bit={b.tell()}")
    n=read_eed(b,ms)
    print(f"after eed n={n} @bit={b.tell()} byte={b.byte} bit={b.bit}")
    seg=data[b.byte:b.byte+ms]
    print("seg:",seg.hex(' '))
    for off in range(0,len(seg)-7):
        d=struct.unpack_from("<d",seg,off)[0]
        if abs(d+300)<1e-6 or abs(d-200)<1e-6 or abs(d+200)<1e-6 or abs(d-0.75)<1e-6:
            print(f"  double@seg+{off} = {d}")
