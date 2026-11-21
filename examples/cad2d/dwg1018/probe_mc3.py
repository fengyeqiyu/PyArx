# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
# entry0 @2: ho=01 mc=04 @4. entries from byte4
p=4
def umc(b,p):
    v=0;k=0
    for _ in range(8):
        x=b[p];p+=1
        if not x&0x80: return v|(x<<k),p
        v|=(x&0x7f)<<k;k+=7
    return 0,p
def mc(b,p):
    neg=0;v=0;k=0
    for _ in range(5):
        x=b[p];p+=1
        if not x&0x80:
            if x&0x40:neg=1;x&=0xbf
            return (-v if neg else v),p
        v|=(x&0x7f)<<k;k+=7
    return 0,p
# real locs: obj0=4 obj1=2806 obj2=2966 obj3=3033 obj4=3061
real=[4,2806,2966,3033,3061]
for i in range(5):
    ho,p=umc(hdl,p);off,p=mc(hdl,p)
    print(i,"ho",ho,"mc",off,"raw@",p,"real_loc",real[i] if i<len(real) else "-")
