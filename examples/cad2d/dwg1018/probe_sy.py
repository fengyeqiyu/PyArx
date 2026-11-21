# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=140531; body=addr-2

# 权威: z_is_zero@30.6, sx@30.7, sy@39.1, ex@47.3
b=BitStream(data); b.byte=addr+30; b.bit=6
zz=b.read_b(); print(f"zz={zz} after {b.byte-addr}.{b.bit}")
sx=b.read_rd(); print(f"sx={sx} after {b.byte-addr}.{b.bit}")
sy=b.read_rd(); print(f"sy={sy} after {b.byte-addr}.{b.bit}")
# 权威 sy@39.1，手动定位
q=BitStream(data); q.byte=addr+39; q.bit=1
print("manual sy@39.1 =", q.read_rd())
# dump 30..48 字节
seg=data[addr+30:addr+48]
print("seg:",seg.hex(' '))
