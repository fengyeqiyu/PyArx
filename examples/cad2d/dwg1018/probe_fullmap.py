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
total=0; matched=0; missing=0; mism=[]
pages=0
while True:
    start=b.byte
    ss=b.read_rs_be()
    if ss>2040:
        print("bad size",ss,"@",start); break
    last_h=0; last_off=0
    end=start+ss
    while b.byte<end:
        old=b.byte
        ho=b.read_umc()
        if not ho: b.byte=old+1; ho=b.read_umc()
        off=b.read_mc()
        last_h+=ho; last_off+=off
        total+=1
        ra=ref.get(last_h)
        if ra is None: missing+=1
        elif last_off==ra-2: matched+=1
        else:
            if len(mism)<15: mism.append((hex(last_h),last_off,ra-2))
    # CRC
    crc=b.read_rs_be()
    pages+=1
    if b.byte>=len(hdl)-2: break
print("pages",pages,"total",total,"matched",matched,"missing",missing,"mismatch",len(mism))
for m in mism: print("  MISM",m)
