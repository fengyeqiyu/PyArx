# -*- coding: utf-8 -*-
import struct
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
a=20367; ms=66; raw=data[a:a+ms+4]
targets=[("sx",0.888997),("sy",0.817017),("ex",0.900028),("ey",0.835484)]
for name,val in targets:
    le=struct.pack("<d",val); be=struct.pack(">d",val)
    il=raw.find(le); ib=raw.find(be)
    print(name,"le@",il,"be@",ib,"lebytes",le.hex(' '))
# also search whole section for sx le bytes
le=struct.pack("<d",0.888997)
idx=data.find(le)
print("whole section sx le@",idx)
# dump raw as doubles at each 8-byte aligned offset
print("--- reinterpret raw as doubles ---")
for off in range(0,len(raw)-7):
    try:
        v=struct.unpack_from("<d",raw,off)[0]
        if -1e6<v<1e6 and v!=0 and abs(v)>1e-6:
            print(f"  off {off}: {v:.6f}")
    except Exception: pass
