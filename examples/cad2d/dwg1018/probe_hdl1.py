# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
b = BitStream(data)
sec = b.read_rs_be()
print("sec_size", sec, "byte", b.byte, "bit", b.bit)
for i in range(6):
    ho = b.read_umc()
    off = b.read_mc()
    print(f"  #{i} handleoff={ho} offset={off} byte={b.byte} bit={b.bit}")
