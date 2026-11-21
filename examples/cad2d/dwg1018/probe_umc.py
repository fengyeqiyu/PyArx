# -*- coding: utf-8 -*-
# 0x4237A - 1 = 0x42379. UMC 编码
v=0x42379
out=[]
while v>=0x80:
    out.append((v&0x7F)|0x80); v>>=7
out.append(v&0x7F)
print("handleoff=0x%X UMC bytes:"%0x42379, [hex(x) for x in out])
# 0x4237A 直接编码?
v=0x4237A
out=[]
while v>=0x80:
    out.append((v&0x7F)|0x80); v>>=7
out.append(v&0x7F)
print("handle=0x4237A UMC bytes:", [hex(x) for x in out])
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
for pat in [bytes([0x79,0x46,0x84]), bytes([0x7A,0x46,0x84])]:
    i=data.find(pat)
    print(pat.hex(), "hits@", i)
