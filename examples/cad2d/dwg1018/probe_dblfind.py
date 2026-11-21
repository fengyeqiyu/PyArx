# -*- coding: utf-8 -*-
import struct
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
for val in [0.888997,0.817017,0.900028,0.835484,464954.458,-1808.776]:
    le=struct.pack("<d",val); be=struct.pack(">d",val)
    il=data.find(le); ib=data.find(be)
    print(f"{val:12.3f} le@{il} be@{ib}")
# count doubles in plausible CAD range: scan aligned 8-byte
import math
cnt=0; ex=[]
for off in range(0,len(data)-8,8):
    v=struct.unpack_from("<d",data,off)[0]
    if 1e-3<abs(v)<1e7 and math.isfinite(v):
        cnt+=1
        if len(ex)<10: ex.append((off,round(v,3)))
print("aligned doubles in range:",cnt, ex)
