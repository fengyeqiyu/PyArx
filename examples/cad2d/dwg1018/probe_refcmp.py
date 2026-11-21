# -*- coding: utf-8 -*-
ref={}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"):
    h,a,t,n=line.rstrip("\n").split("\t",3)
    ref[int(h,16)]=(int(a),t)
# 我之前解析的对象节顺序
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    c=st.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|st.read_rc()
    return v
addr=4; mine=[]
for _ in range(12):
    b=BitStream(data); b.byte=addr
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    mine.append((addr,h,ot,ms)); addr+=ms+4
for a,h,ot,ms in mine:
    ra,rt=ref.get(h,(-1,"?"))
    print(f"mine addr={a} h=0x{h:X} ot=0x{ot:X} | ref addr={ra} type={rt} diff={ra-a}")
