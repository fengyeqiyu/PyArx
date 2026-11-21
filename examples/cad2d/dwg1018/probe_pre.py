# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("first 32:",data[:32].hex(' '))
# object0 @4 ms=2798; inspect bytes 0..4
import struct
print("RL @0=",hex(struct.unpack_from('<I',data,0)[0]))
# try parse object starting at 0
for start in [0,4,8]:
    b=BitStream(data); b.byte=start
    try:
        ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl()
        print(f"start={start} ms={ms} ot={hex(ot)} rl={rl} pos={b.byte}.{b.bit}")
    except Exception as e:
        print(start,"err",e)
