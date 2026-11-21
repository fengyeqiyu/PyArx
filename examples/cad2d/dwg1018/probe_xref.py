# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
obj=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()

def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v

# object chain
addr=4; objs=[]
for i in range(10):
    b=BitStream(obj); b.byte=addr
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
    objs.append((addr,ms,ot,h))
    addr=addr+ms+4
print("=== objects ===")
for i,(a,ms,ot,h) in enumerate(objs):
    print(i,"addr",a,"size",ms,"ot",hex(ot),"h",hex(h),"next_off(size+4)",ms+4)

# handle map first entries
st=BitStream(hdl)
ss=st.read_rs_be(); lo=0; lh=0
print("=== handle map section_size",ss)
for i in range(10):
    ho=st.read_umc(); off=st.read_mc()
    lh+=ho; lo+=off
    print(i,"h",lh,"handleoff",ho,"mc",off,"last_off",lo)
