# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()

def umc(i):
    res=0;j=0
    while i<len(data):
        b=data[i];i+=1
        if not b&0x80: return res|b<<j,i
        res|=(b&0x7F)<<j;j+=7
    return res,i
def mc(i):
    neg=0;res=0;j=0
    for _ in range(5):
        b=data[i];i+=1
        if not b&0x80:
            if b&0x40: neg=1;b&=0xBF
            return (-res if neg else res)|(b<<j),i
        res|=(b&0x7F)<<j;j+=7
    return 0,i

sec=(data[0]<<8)|data[1]
i=2; h=0; off=0
print("sec",sec)
for k in range(6):
    ho,i=umc(i); offv,i=mc(i)
    h+=ho; off+=offv
    print(f"#{k} @{i} ho={ho} off={offv} handle=0x{h:X} addr={off}")
