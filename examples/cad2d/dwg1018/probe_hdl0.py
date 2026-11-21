# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
print("first 40:",data[:40].hex(' '))
st=BitStream(data)
ss=st.read_rs_be(); print("section_size",ss,"pos",st.byte,st.bit)
for i in range(6):
    ho=st.read_umc(); off=st.read_mc()
    print(i,"ho",ho,"mc",off,"pos",st.byte,st.bit)
