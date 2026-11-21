# -*- coding: utf-8 -*-
payload = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_body.bin","rb").read()
for i in range(5128, 5146):
    print(i, hex(payload[i]))
