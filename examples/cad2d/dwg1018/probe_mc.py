# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()

def umc(bs,i):
    res=0;j=0
    while i<len(bs):
        b=bs[i];i+=1
        if not b&0x80: res|=b<<j; return res,i
        res|=(b&0x7F)<<j;j+=7
    return res,i

def mc(bs,i):
    neg=0;res=0;j=0
    for _ in range(5):
        b=bs[i];i+=1
        if not b&0x80:
            if b&0x40: neg=1;b&=0xBF
            res|=b<<j; return (-res if neg else res),i
        res|=(b&0x7F)<<j;j+=7
    return 0,i

# 首条: ho@2=01, off@3=04 -> 结束@4
# #1 ho@4=01 -> 结束@5; offset 从@5
for label,fn in [("MC",mc),("UMC",umc)]:
    v,i=fn(data,5)
    print(label, "offset=",v, "end@",i)

# 期望 2802 编码: 2802 = 0xAE2
# 2802 low7=0x62 (0x22? ) 手工: 2802 = 0b101011100010
print("2802 hex", hex(2802), "b0", 2802&0x7F, "b1", (2802>>7)&0x7F)
