# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
ref={}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"):
    h,a,t,n=line.rstrip("\n").split("\t",3)
    ref[int(h)]=(int(a),t,n)
for hx in ("11CF7",):
    h=int(hx,16); a,t,n=ref[h]; body=a-2
    b=BitStream(data); b.byte=body
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl()
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    print(f"h={hx} ref_addr={a} body={body} ms={ms} type=0x{ot:X}({n}) h=0x{v:X} match={v==h}")
    print("bytes:",data[body:body+20].hex(' '))
