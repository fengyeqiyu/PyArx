# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
b=BitStream(hdl)
ss=b.read_rs_be(); print("sec0 ss",ss)
# 跳完 sec0
while b.byte<ss:
    old=b.byte; ho=b.read_umc()
    if not ho: b.byte=old+1; ho=b.read_umc()
    b.read_mc()
b.read_rs_be()
print("sec1 @",b.byte,"bytes",hdl[b.byte:b.byte+12].hex(' '))
ss1=b.read_rs_be(); print("sec1 ss",ss1)
for i in range(4):
    old=b.byte
    ho=b.read_umc()
    off=b.read_mc()
    print(f"  #{i} @{old} ho={ho} off={off}")
