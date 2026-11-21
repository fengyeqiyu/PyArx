# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
print("first 64:", data[:64].hex(' '))
print("bytes 2030-2040:", data[2030:2040].hex(' '))
