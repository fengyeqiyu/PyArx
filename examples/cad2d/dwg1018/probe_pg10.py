# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
print("len",len(hdl))
p=0; pg=0
while p+2<=len(hdl):
    ss=(hdl[p]<<8)|hdl[p+1]
    if ss>2040:
        print(f"page {pg}: BAD ss={ss} @{p} bytes={hdl[p:p+8].hex(' ')}"); break
    end=p+ss
    crc=(hdl[end]<<8)|hdl[end+1]
    print(f"page {pg}: ss={ss} @{p} end={end} crc={crc:04X}")
    p=end+2; pg+=1
