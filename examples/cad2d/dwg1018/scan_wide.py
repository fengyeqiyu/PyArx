# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
hits=[]
for off in range(0, 4096):
    b = BitStream(data); b.byte=off; b.bit=0
    try:
        ms=b.read_ms(); bs=b.read_bs()
    except Exception:
        continue
    if (1<=bs<=31 or bs==256) and 0<ms<=4000:
        hits.append((off,ms,bs))
for h in hits[:40]:
    print(h)
print("total hits", len(hits))
