# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

# ref: h=85 addr=140531 start=(-300,200) end=(-300,-200) layer=6 color=256
addr=140531; body=addr-2
b=BitStream(data); b.byte=body
ms=b.read_ms(); ot=b.read_bs(); bitsize=b.read_rl()
c=b.read_rc(); sz=c&0xf; hv=0
for _ in range(sz): hv=(hv<<8)|b.read_rc()
print(f"ms={ms} type=0x{ot:X} bitsize={bitsize} h=0x{hv:X} @bit={b.tell()}")
# EED 解码（源码 dwg_decode_eed）：先 BL 计数
def read_eed(b):
    n=b.read_bl()
    print(f"  eed num={n}")
    for i in range(min(n,10)):
        app=b.read_rc()
        sz=b.read_bs()
        raw=data[b.byte:b.byte+sz] if b.bit==0 else b.read_tf(sz)
        print(f"    eed{i} app={app} sz={sz} raw={raw[:16].hex(' ')}")
    return n

read_eed(b)
print(f"after eed @bit={b.tell()} byte={b.byte}")
# 扫描后续缓冲中可读 double，找 -300 / 200
seg=data[b.byte:b.byte+ms]
print("seg hex:",seg.hex(' '))
for off in range(0,len(seg)-7):
    u=struct.unpack_from("<Q",seg,off)[0]
    d=struct.unpack_from("<d",seg,off)[0]
    if abs(d+300)<1e-6 or abs(d-200)<1e-6 or abs(d-(-200))<1e-6:
        print(f"  double@seg+{off} = {d} u=0x{u:X}")
