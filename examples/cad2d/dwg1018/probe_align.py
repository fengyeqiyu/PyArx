# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return code,size,v

# collect first few LINE addrs
addr=4; lines=[]
while addr<len(data)-4:
    b=BitStream(data); b.byte=addr
    try: ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); c,s,h=read_h(b)
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    if ot==0x13: lines.append((addr,ms,rl,h))
    addr=addr+ms+4

def parse(a, align_eed, after_alg=False):
    b=BitStream(data); b.byte=a
    b.read_ms(); b.read_bs(); b.read_rl(); read_h(b)
    while True:
        sz=b.read_bs()
        if not sz: break
        read_h(b); b.read_tf(sz)
    if align_eed:
        if b.bit: b.byte+=1; b.bit=0
    flag=b.read_rc()
    out=[f"flag={flag:#x}"]
    if flag:
        objsize=b.read_rs(); out.append(f"esize={objsize}")
        cc,ss,layer=read_h(b); out.append(f"layer={layer:#x}(c{cc}s{ss})")
    opts=b.read_rs(); out.append(f"opts={opts:#x}")
    if flag&64: b.read_rc()
    if flag&1:
        col=b.read_rc(); out.append(f"color={col}")
    if flag&2:
        cc,ss,lt=read_h(b); out.append(f"ltype={lt:#x}")
    if flag&8: b.read_rd()
    if flag&32: read_h(b)
    prev=b.read_b(); out.append(f"prev={prev}")
    entmode=b.read_bb(); nreact=b.read_bl()
    out.append(f"entmode={entmode} nreact={nreact}")
    return " ".join(out), b.byte, b.bit

for a,ms,rl,h in lines[:4]:
    print(f"LINE @{a} ms={ms} rl={rl} h={h:#x}")
    for al in (False,True):
        s,by,bit=parse(a,al)
        print(f"  align_eed={al}: {s}  end {by}.{bit}")
