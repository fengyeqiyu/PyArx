# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()

# section header: 4 RL? read container to know. Just dump first bytes
print("first 32:",data[:32].hex(' '))
st=BitStream(data)
ss=st.read_rs_be()
print("section_size",ss)
last_off=0; last_h=0
for i in range(12):
    ho=st.read_umc(); off=st.read_mc()
    last_h+=ho; last_off+=off
    print(i,"handle",last_h,"handleoff",ho,"mc_off",off,"last_off",last_off)
