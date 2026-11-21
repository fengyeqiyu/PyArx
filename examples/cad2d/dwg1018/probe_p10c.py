# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
exec(open(r"G:\PyArx\examples\cad2d\dwg1018\parse_hdl.py").read().split("stream =")[0])
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
# 页10 起始偏移 296960
off=296960
b=BitStream(data); b.byte=off
sec=b.read_rs_be()
print("p10 sec_size",sec,"start",off,"end",off+sec)
body=data[off:off+sec]
print("first 24:", body[:24].hex(' '))
calc=crc16(body)
rd=(data[off+sec]<<8)|data[off+sec+1]
rd2=data[off+sec]|(data[off+sec+1]<<8)
print("calc",f"{calc:04X}","read BE",f"{rd:04X}","read LE",f"{rd2:04X}")
# 统计非零
print("nonzero", sum(1 for x in body if x))
