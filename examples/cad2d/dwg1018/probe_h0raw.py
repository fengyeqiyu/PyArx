# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
print("len",len(hdl))
print("0..80:",hdl[:80].hex(' '))
def rs_be(p): return (hdl[p]<<8)|hdl[p+1]
ss=rs_be(0); print("section_size",ss)
p=2
last_h=0;last_off=0
for i in range(8):
    start=p
    # UMC
    v=0;j=0;nb=0
    while True:
        x=hdl[p];p+=1;nb+=1
        if not x&0x80: v|=x<<j; break
        v|=(x&0x7f)<<j;j+=7
    ho=v
    # MC
    neg=0;v=0;j=0;nb2=0
    while True:
        x=hdl[p];p+=1;nb2+=1
        if not x&0x80:
            if x&0x40: neg=1;x&=0xbf
            v|=x<<j;break
        x&=0x7f;v|=x<<j;j+=7
    off=-v if neg else v
    last_h+=ho;last_off+=off
    print(i,"@",start,"UMC",ho,"(",nb,"B) MC",off,"(",nb2,"B) h=",last_h,"loc=",last_off)
