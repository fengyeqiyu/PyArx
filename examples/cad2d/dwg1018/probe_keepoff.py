# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
ref={}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"):
    h,a,t,n=line.rstrip("\n").split("\t",3)
    ref[int(h)]=int(a)
b=BitStream(hdl); last_h=0; last_off=0  # 跨节持续
for sec in range(3):
    start=b.byte; ss=b.read_rs_be(); end=start+ss; n=0; first=None
    while b.byte<end:
        old=b.byte; ho=b.read_umc()
        if not ho: b.byte=old+1; ho=b.read_umc()
        off=b.read_mc(); last_h+=ho; last_off+=off
        if first is None: first=(last_h,last_off)
        n+=1
    b.read_rs_be()
    fh,fo=first; ra=ref.get(fh)
    print(f"sec {sec} n={n} first h=0x{fh:X} body={fo} ref={ra} {'OK' if ra and fo==ra-2 else 'X'}")
