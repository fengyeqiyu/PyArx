# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
st=BitStream(data)
sec=st.read_rs_be()
print("page0 section_size",sec,"@",st.byte)
start=st.byte
last_h=0; last_off=0
for i in range(8):
    ho=st.read_umc(); off=st.read_mc()
    last_h+=ho; last_off+=off
    print(f"  entry{i} handleoff={ho} offset={off} -> handle={last_h} loc={last_off}")
