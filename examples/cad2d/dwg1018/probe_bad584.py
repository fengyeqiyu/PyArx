# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
exec(open(r"G:\PyArx\examples\cad2d\dwg1018\probe_objcrc.py").read().split("addr=4")[0])
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
addr=4; n=0; objs=[]
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try: ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    objs.append((n,addr,ms,ot,rl,h)); n+=1; addr=addr+ms+4
for t in objs[582:587]:
    i,a,ms,ot,rl,h=t; end=a+ms+4
    calc=crc16(data[a:end-2]); rd=data[end-2]|(data[end-1]<<8)
    print(f"#{i} @{a} ms={ms} ot={ot:#x} rl={rl} h={h:#x} crc {calc:04X}/{rd:04X}")
# dump 584 raw
i,a,ms,ot,rl,h=objs[584]
print("raw:",data[a:a+ms].hex(' '))
