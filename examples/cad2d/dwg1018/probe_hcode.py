# -*- coding: utf-8 -*-
hdl = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
obj = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def find(bs, pat, n=8):
    hits=[]; i=0
    while True:
        j=bs.find(pat,i)
        if j<0: break
        hits.append(j); i=j+1
    return hits[:n]

# 0x4237A 大端 3 字节: 42 37 7A, H code size=3 -> 0x03
print("H 0x4237A enc 03 42 37 7A in hdl:", find(hdl, bytes([3,0x42,0x37,0x7A])))
print("H 0x4237A enc in obj:", find(obj, bytes([3,0x42,0x37,0x7A])))
# 0x423D5
print("H 0x423D5 in obj:", find(obj, bytes([3,0x42,0x3D,0xD5])))
# 句柄表前 64 字节
print("hdl[:64]:", hdl[:64].hex(' '))
