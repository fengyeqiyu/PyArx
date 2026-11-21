# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
ref={}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"):
    h,a,t,n=line.rstrip("\n").split("\t",3)
    ref[int(h)]=int(a)
b=BitStream(hdl)
ss=b.read_rs_be(); start=b.byte; last_h=0; last_off=0; n=0
while b.byte-start<ss:
    old=b.byte
    ho=b.read_umc()
    if not ho: b.byte=old+1; ho=b.read_umc()
    off=b.read_mc()
    last_h+=ho; last_off+=off
    ra=ref.get(last_h)
    if 8<=n<=20:
        print(f"#{n} @{old} ho={ho} off={off} h=0x{last_h:X} body={last_off} ref={ra}")
    n+=1
