# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
P=29696
def read_h(st):
    code=st.read_rc();size=code&0xF;v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
for pg in range(0,12):
    base=pg*P; addr=base; n=0; handles=[]
    while addr<base+P-10:
        b=BitStream(data); b.byte=addr
        try:
            ms=b.read_ms();ot=b.read_bs();rl=b.read_rl();h=read_h(b)
        except Exception: break
        if ms<=0 or ms>0x8000 or ot>0x1f4: break
        n+=1; handles.append(h); addr=addr+ms+4
        if addr>base+P: break
    print(f"page {pg} base={base} chain={n} end={addr} maxh={hex(max(handles)) if handles else '-'}")
