# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
print("2035:",hdl[2035:2035+14].hex(' '))
# ss=07 f1 @2035; 条目 @2037
p=2037
# 逐字节标 UMC/MC
def umc(b,p):
    v=0;j=0;bs=[]
    while True:
        x=b[p];p+=1;bs.append(x)
        if not x&0x80: return v|(x<<j),p,bs
        v|=(x&0x7f)<<j;j+=7
def mc(b,p):
    neg=0;v=0;j=0;bs=[]
    while True:
        x=b[p];p+=1;bs.append(x)
        if not x&0x80:
            if x&0x40: neg=1;x&=0xbf
            v|=x<<j;return (-v if neg else v),p,bs
        x&=0x7f;v|=x<<j;j+=7
ho,p1,bs1=umc(hdl,p); off,p2,bs2=mc(hdl,p1)
print("UMC",ho,[hex(x) for x in bs1],"@",p,"->",p1)
print("MC",off,[hex(x) for x in bs2],"@",p1,"->",p2)
