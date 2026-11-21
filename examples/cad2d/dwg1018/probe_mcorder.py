# -*- coding: utf-8 -*-
# 按源码 read 槽位规则（byte[i] 末组落槽 i，槽 k 占 7k 位）反推 160 的编码字节顺序
def encode_bigend(v):
    neg=v<0; u=abs(v)
    byte=[0]*5
    for i in range(4,-1,-1):
        byte[i]=u&0x7f; u>>=7
    i=0
    while i<4 and not(byte[i]&0x7f): i+=1
    if byte[i]&0x40 and i>0: i-=1
    byte[i]&=0x7f
    if neg: byte[i]|=0x40
    # read 从槽 4 往下读；write 实际写出顺序待对照
    return byte, i
for v in (4,160,2802,67):
    b,i=encode_bigend(v)
    print(v,"slots",[hex(x) for x in b],"first-nonzero i=",i)
