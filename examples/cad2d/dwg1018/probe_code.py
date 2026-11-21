# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
# 2802 modular: b0=0x72(114) -> 0x72|0x80=0xF2, b1=21 ->0x15
print("expect bytes 2802:", (2802&0x7F)|0x80, (2802>>7)&0x7F)
pat=bytes([0xF2,0x15])
idx=0
hits=[]
while True:
    j=data.find(pat,idx)
    if j<0: break
    hits.append(j); idx=j+1
print("F2 15 hits:", hits[:10])

# 2806-4=2802; 但首条 offset=4 本身可能是相对0。列出前 40 字节
print(data[:40].hex(' '))
