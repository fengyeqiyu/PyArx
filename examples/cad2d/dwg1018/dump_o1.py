# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
for addr in [4, 2806, 2966]:
    print(f"@{addr}:", data[addr:addr+20].hex(' '))
