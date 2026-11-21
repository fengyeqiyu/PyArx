# -*- coding: utf-8 -*-
full=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
ref=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_ref.bin","rb").read()
# 0..996 一致, 997 起差异。打印 960..1060 对比
for i in range(960,1060,16):
    f=full[i:i+16].hex(' ')
    r=ref[i:i+16].hex(' ')
    mark="" if f==r else "  <<"
    print(f"@{i:4d} F {f}")
    print(f"       R {r}{mark}")
