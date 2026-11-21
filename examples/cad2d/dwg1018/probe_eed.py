# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    code=b.read_rc(); sz=code&0xf
    v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return code,sz,v

body=140529
b=BitStream(data); b.byte=body
ms=b.read_ms(); ot=b.read_bs(); bitsize=b.read_rl()
c=b.read_rc(); hsz=c&0xf; hv=0
for _ in range(hsz): hv=(hv<<8)|b.read_rc()
print(f"ms={ms} type=0x{ot:X} bitsize={bitsize} h=0x{hv:X} @{b.tell()}")

def eed_field(b, code, size, used):
    if code==70: v=b.read_rs(); used[0]+=2; return f"short={v}"
    if code==71: v=b.read_rl(); used[0]+=4; return f"long={v}"
    if code==5: v=b.read_rll_be(); used[0]+=8; return f"entity=0x{v:X}"
    if code==1: v=b.read_rs(); used[0]+=2; return f"appidx={v}"
    if code==3: v=b.read_rll_be(); used[0]+=8; return f"layer=0x{v:X}"
    if code==2: v=b.read_rc(); used[0]+=1; return f"close={v}"
    if code==0:
        l=b.read_rc(); used[0]+=1; b.read_tf(l); used[0]+=l; return f"str len={l}"
    if code==4:
        l=b.read_rc(); used[0]+=1; b.read_tf(l); used[0]+=l; return f"bin len={l}"
    if code in (10,11,12,13,14,15):
        p=tuple(b.read_rd() for _ in range(3)); used[0]+=24; return f"pt={p}"
    if code in (40,41,42):
        v=b.read_rd(); used[0]+=8; return f"real={v}"
    return f"??code={code}"

# EED 外层：BS size -> H handle -> size 字节数据（位流内）
while True:
    size=b.read_bs()
    if not size: break
    code,hsz,hv=read_h(b)
    used=[0]
    print(f"eed size={size} hcode={code} hsz={hsz} h=0x{hv:X} @{b.tell()}")
    end_bits=b.tell()+size*8
    while b.tell()<end_bits:
        c=b.read_rc()
        print("   ", eed_field(b,c,size,used), f"@{b.tell()}")
    # 外层：下一条 EED 从 end 开始；size 已含本条目全部数据
print(f"after eed @{b.tell()}")
