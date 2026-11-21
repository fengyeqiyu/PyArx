# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
data = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()

def umc(bs, i):
    res=0; j=0
    while i < len(bs):
        b=bs[i]; i+=1
        if not b&0x80:
            res|=b<<j; return res,i
        res|=(b&0x7F)<<j; j+=7
    return res,i

def mc(bs,i):
    neg=0; res=0; j=0
    for _ in range(5):
        b=bs[i]; i+=1
        if not b&0x80:
            if b&0x40: neg=1; b&=0xBF
            res|=b<<j; return (-res if neg else res),i
        res|=(b&0x7F)<<j; j+=7
    return 0,i

# sec_size 2 bytes BE
sec=(data[0]<<8)|data[1]
print("sec_size",sec)
i=2
last_handle=0; last_off=0
for k in range(8):
    ho,i=umc(data,i)
    off,i=mc(data,i)
    last_handle+=ho; last_off+=off
    print(f"#{k} @{i} ho={ho} off={off} handle={last_handle} addr={last_off}")
