# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc();size=code&0xF;v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
# scan whole section, collect plausible object headers, handle->addr
# walk chain only; also brute scan for H code 01 02 preceded by valid MS/BS
for hwant in [2,3,5]:
    code=bytes([1])+hwant.to_bytes(1,'big')
    idx=0; locs=[]
    while True:
        i=data.find(code,idx)
        if i<0: break
        # check plausible header at i-? : H follows MS,BS,RL
        locs.append(i); idx=i+1
    print(hex(hwant),"candidate H@",locs[:10])
