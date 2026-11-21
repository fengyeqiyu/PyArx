# -*- coding: utf-8 -*-
# 复刻源码 read_MC（byte[i] 槽位, result|=byte[i]<<j）与 write_MC，闭环
def write_mc(v):
    neg=v<0; u=abs(v); byte=[0]*5
    for i in range(4,-1,-1):
        byte[i]=u&0x7f; u>>=7
    i=0
    while i<4 and not(byte[i]&0x7f): i+=1
    if byte[i]&0x40 and i>0: i-=1
    byte[i]&=0x7f
    if neg: byte[i]|=0x40
    # write 从 j=4 往 i 写（高位槽先出）
    out=list(reversed(byte[i:]))
    for k in range(i,5): byte[k]|=0x80
    return out

def read_mc(bs, p):
    neg=0; res=0
    for i,j in zip(range(4,-1,-1),range(0,35,7)):
        b=bs[p]; p+=1
        if not b&0x80:
            if b&0x40: neg=1; b&=0xbf
            res|=b<<j; return (-res if neg else res),p
        b&=0x7f; res|=b<<j
    return 0,p

for v in (4,160,2802,67,1073,12345,-63):
    enc=write_mc(v)
    dec,p=read_mc(enc,0)
    print(v,"enc",[hex(x) for x in enc],"read",dec,"OK" if dec==v else "BAD")
