# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
# sec0 end=2033, crc@2033-2034, sec1 @20352? 实际 sec1 @20352 per probe
for p in (20352,20350,20351,20349):
    print(p, hdl[p:p+10].hex(' '))
