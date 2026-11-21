# -*- coding: utf-8 -*-
data=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
for off in [296960, 296960-2, 296960-16, 296960-32]:
    wbe=(data[off]<<8)|data[off+1]
    wle=data[off]|data[off+1]<<8
    print(f"@{off} BE={wbe} LE={wle} bytes={data[off:off+6].hex(' ')}")
