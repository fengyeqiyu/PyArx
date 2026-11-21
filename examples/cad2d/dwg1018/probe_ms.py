# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("bytes @4:", data[4:10].hex(' '))
# RS LE: w1=data[4]|data[5]<<8, w2=data[6]|data[7]<<8
w1=data[4]|data[5]<<8; w2=data[6]|data[7]<<8
print(f"w1=0x{w1:04X} w2=0x{w2:04X}")
print("w1 hi bit?", bool(w1&0x8000), "w2 hi?", bool(w2&0x8000))
# 源码: 首读字 j=0 低位, 次读字 j=15 高位
res1=(w1&0x7FFF)|((w2&0x7FFF)<<15)
# 反: 首字高位, 次字低位
res2=(w2&0x7FFF)|((w1&0x7FFF)<<15)
print("j=0首低:", res1, " j=0首高:", res2)
print("target 2802=",2802, "2798=",2798)
