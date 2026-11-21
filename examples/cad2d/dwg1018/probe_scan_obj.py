# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
N=len(data)
print("obj size", N)

def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v

# 从候选地址解析对象头，返回 (ms,ot,rl,h,eed) 或 None
def try_obj(addr):
    bb=BitStream(data)
    try:
        bb.byte=addr; bb.bit=0
        ms=bb.read_ms(); ot=bb.read_bs(); rl=bb.read_rl(); h=read_h(bb); eed=bb.read_bs()
    except Exception:
        return None
    if not(0<=ot<0x1f4 and 0<h<0x100000 and 0<=eed<0x200 and ms>0 and ms<0x40000 and rl>=0):
        return None
    return ms,ot,rl,h,eed

# 首对象 @4
r0=try_obj(4)
print("obj0", r0)
ms0=r0[0]
print("addr after obj0 =", 4+ms0+4)

# 在首对象结尾附近 ±20 扫描第二对象
base=4+ms0+4
for addr in range(base-20, base+20):
    r=try_obj(addr)
    if r: print(f"  cand @{addr} MS={r[0]} OT={r[1]} RL={r[2]} H=0x{r[3]:X} EED={r[4]}")
