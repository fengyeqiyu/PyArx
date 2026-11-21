# -*- coding: utf-8 -*-
# 真实对象顺序（句柄表前几条）：addr 4,2806,2966,3033,... 增量 4,2802,160,67
# 对照 hdl 页0 字节: 07 f1 | 01 04 01 d9 b9 a5 09 01 f5 8d c0 00 02 24 01 b0 c0 ...
def mc_encode(v):
    if v<0: neg=1; u=-v
    else: neg=0; u=v
    byte=[0]*5
    for i in range(4,-1,-1):
        byte[i]=u&0x7f; u>>=7
    i=0
    while i<4 and not (byte[i]&0x7f): i+=1
    if byte[i]&0x40 and i>0: i-=1
    byte[i]&=0x7f
    if neg: byte[i]|=0x40
    out=byte[i:]
    for k in range(i,5): byte[k]|=0x80
    return byte[i:], out
for v in (4,2802,160,67,42):
    b,_=mc_encode(v)
    print(v, [hex(x|0x80) for x in b], "end", hex(b[-1]&0x3f))
