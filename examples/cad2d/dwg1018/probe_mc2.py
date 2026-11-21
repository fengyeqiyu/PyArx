# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
print("bytes 2..30:",hdl[2:30].hex(' '))
def mc_be(b,p):
    groups=[]
    for _ in range(5):
        x=b[p];p+=1;groups.append(x)
        if not x&0x80: break
    v=0
    for g in reversed(groups):
        v=(v<<7)|(g&0x7f)
    if groups[-1]&0x40: v=-v
    return v,p
p=2
for i in range(6):
    ho=0;k=0
    for _ in range(8):
        x=hdl[p];p+=1
        if not x&0x80: ho|=x<<k;break
        ho|=(x&0x7f)<<k;k+=7
    off,p=mc_be(hdl,p)
    print(i,"ho",ho,"mc_be",off,"p",p)
