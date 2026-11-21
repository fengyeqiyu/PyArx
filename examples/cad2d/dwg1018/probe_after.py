# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("len",len(data))
for off in [0,4,97795,97796,100000,200000,500000,1000000,5000000,9000000,19488853]:
    print(off, data[off:off+16].hex(' '))
# nonzero extent after 97795
nz=[i for i in range(97795,len(data)) if data[i]]
print("nonzero after 97795:",len(nz),"first",nz[:3],"last",nz[-3:] if nz else None)
# does 0x11CF7 handle bytes appear near 19488853
print("around 19488853:",data[19488840:19488870].hex(' '))
