# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
ref={}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"):
    h,a,t,n=line.rstrip("\n").split("\t",3)
    ref[int(h,16)]=(int(a),t)
for hx in ("1","2","3","5","4237A"):
    h=int(hx,16); a,t=ref[h]
    print(f"h={hx} ref addr={a} type={t}")
    for off in (0,-2):
        p=a+off
        if p<0: continue
        b=BitStream(data); b.byte=p
        try:
            ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl()
            c=b.read_rc(); sz=c&0xf; v=0
            for _ in range(sz): v=(v<<8)|b.read_rc()
            print(f"   addr{p} ms={ms} type=0x{ot:X} h=0x{v:X}")
        except Exception as e: print("   err",e)
