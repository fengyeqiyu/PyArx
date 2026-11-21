# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
exec(open(r"G:\PyArx\examples\cad2d\dwg1018\probe_objcrc.py").read().split("addr=4")[0])
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
from bitio import BitStream
addr=4; idx=0; bad=0
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    except Exception: break
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if end>=len(data): break
    crc_calc=crc16(data[addr:end-2]); crc_read=data[end-2]|(data[end-1]<<8)
    ok=crc_calc==crc_read
    if not ok: bad+=1
    if ot==0x13 and idx<156:
        print(f"idx={idx} addr={addr} ms={ms} ot={ot} crc={'OK' if ok else 'BAD'} calc={crc_calc:04X} read={crc_read:04X}")
    idx+=1; addr=end
print("total",idx,"bad",bad)
