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

addr=4; n=0; bad=[]; types={}
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    except Exception as e:
        bad.append((n,addr,"decode:"+str(e))); break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000):
        bad.append((n,addr,f"invalid ms={ms} ot={ot:#x} h={h:#x}")); break
    end=addr+ms+4
    if end>len(data): bad.append((n,addr,f"overrun end={end}")); break
    calc=crc16(data[addr:end-2]); read=data[end-2]|(data[end-1]<<8)
    ok=calc==read
    types[ot]=types.get(ot,0)+1
    if not ok: bad.append((n,addr,f"ms={ms} ot={ot:#x} crc {calc:04X}/{read:04X}"))
    n+=1; addr=end
print("objects",n,"end",addr)
print("bad",len(bad))
for x in bad[:10]: print("  ",x)
