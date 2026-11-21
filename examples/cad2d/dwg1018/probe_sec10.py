# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
p=0; sec=0
while p+2<637399 and sec<12:
    ss=(hdl[p]<<8)|hdl[p+1]
    end=p+ss
    crc=(hdl[end]<<8)|hdl[end+1]
    print(f"sec {sec}: ss={ss} @{p} end={end} crc={crc:04X} next@{end+2} nextbytes={hdl[end+2:end+6].hex(' ')}")
    p=end+2; sec+=1
