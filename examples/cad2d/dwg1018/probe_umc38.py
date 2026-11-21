# -*- coding: utf-8 -*-
def umc_lowfirst(bs):
    v=0;j=0
    for x in bs:
        if not x&0x80: return v|(x<<j)
        v|=(x&0x7f)<<j;j+=7
def umc_highfirst(bs):
    v=0
    for x in bs: v=(v<<7)|(x&0x7f)
    return v
for bs in ([0xfd,0x38],[0xed,0xdf,0x82,0x02]):
    print([hex(x) for x in bs],"lowfirst",umc_lowfirst(bs),"highfirst",umc_highfirst(bs))
# 0x2C0-0x57F 差
print("0x57F-0x2BF=",0x57F-0x2BF)
