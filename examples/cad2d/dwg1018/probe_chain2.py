# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
exec(open(r"G:\PyArx\examples\cad2d\dwg1018\probe_objcrc.py").read().split("addr=4")[0])
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v

addr=4
for k in range(8):
    b=BitStream(data); b.byte=addr
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
    end=addr+ms+4
    calc=crc16(data[addr:end-2]); read=data[end-2]|(data[end-1]<<8)
    crc = "OK" if calc==read else f"bad {calc:04X}/{read:04X}"
    print(f"#{k} @{addr} MS={ms} OT={ot} RL={rl} H=0x{h:X} EED={eed} end={end} CRC={crc}")
    addr=end
