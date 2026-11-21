# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
for i in range(2, 16):
    b=data[i]
    print(f"{i:3d} {b:02X}  hi={1 if b&0x80 else 0} sign={1 if b&0x40 else 0} low7={b&0x7F}")
