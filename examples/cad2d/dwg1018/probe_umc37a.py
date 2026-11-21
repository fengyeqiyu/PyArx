# -*- coding: utf-8 -*-
# 0x4237A 编码：低位组先出还是高位组先出？
v=0x4237A
byte=[0]*8
for i in range(7,-1,-1):
    byte[i]=v&0x7f; v>>=7
print("slots:",[hex(x) for x in byte])
# write 从高位槽 7 往首个非零槽倒序
nz=next(i for i in range(8) if byte[i]&0x7f)
out_high=list(reversed(byte[nz:]))
# 先读落低位（Python 现行）：低位槽先出
out_low=byte[nz:]
print("high-first:",[hex(x|0x80) for x in out_high])
print("low-first :",[hex(x|0x80) for x in out_low])
# 对象1 头里 hcode=0x3, 后随 3 字节
obj=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("obj@2806 head:",obj[2806:2806+12].hex(' '))
