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
from collections import Counter
addr=4; n=0; bad=0; types=Counter()
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if end>=len(data): break
    calc=crc16(data[addr:end-2]); rd=data[end-2]|(data[end-1]<<8)
    if calc!=rd: bad+=1
    types[ot]+=1; n+=1; addr=end
print("objects",n,"bad",bad,"end",addr)
for t,c in types.most_common(): print(f"  OT={t} (0x{t:X}) x{c}")
