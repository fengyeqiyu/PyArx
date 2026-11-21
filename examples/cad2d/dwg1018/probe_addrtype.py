# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
def umc(b,p):
    v=0;k=0
    for _ in range(8):
        x=b[p];p+=1
        if not x&0x80: return v|(x<<k),p
        v|=(x&0x7f)<<k;k+=7
    return 0,p
p=2
for i in range(5):
    ho,p=umc(hdl,p)
    raw=hdl[p:p+5]
    rs_be=(raw[0]<<8)|raw[1]
    rs_le=raw[0]|(raw[1]<<8)
    rl=raw[0]|(raw[1]<<8)|(raw[2]<<16)|(raw[3]<<24)
    print(i,"ho",ho,"raw",raw[:4].hex(),"RS_BE",rs_be,"RS_LE",rs_le,"RL",rl)
    p+=2  # tentative RS
