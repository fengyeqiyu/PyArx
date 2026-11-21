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
last_h=0
for sec in range(12):
    start=b.byte; ss=b.read_rs_be(); end=start+ss; last_off=0; n=0
    while b.byte<end:
        old=b.byte; ho=b.read_umc()
        if not ho: b.byte=old+1; ho=b.read_umc()
        off=b.read_mc(); last_h+=ho; last_off+=off; n+=1
    b.read_rs_be()
    if sec>=8:
        ra=ref.get(last_h)
        print(f"sec {sec} entries={n} last_h=0x{last_h:X} last_off={last_off} ref_last={ra}")
