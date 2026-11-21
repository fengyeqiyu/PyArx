# -*- coding: utf-8 -*-
obj = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
# 单字节 H: code=size=1 -> 首字节=0x01, 次字节=handle
# 搜索 01 后接 2..20
for h in range(1,21):
    pat=bytes([1,h])
    i=obj.find(pat,0,4000)
    if i>=0:
        print(f"H {h}: @{i}  ctx={obj[i-3:i+4].hex(' ')}")
