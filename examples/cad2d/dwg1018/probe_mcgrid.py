# -*- coding: utf-8 -*-
# 真实页0: 07 f1 | 01 04 | 01 d9 b9 a5 09 | 01 f5 8d c0 00 ...
# 已知: entry0 ho=1 off=4 loc=4; entry1 真实 loc=2806 => off=2802; entry2 loc=2966 => off=160
# 候选: 地址字段是 MC(有符号7位组) / UMC(无符号) / RL(小端32) / RS_BE / RC
import struct
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
def umc(b,p):
    v=0;j=0
    while True:
        x=b[p];p+=1
        if not x&0x80: return v|(x<<j),p
        v|=(x&0x7f)<<j;j+=7
p=2
exp=[4,2802,160,67]
for n in range(4):
    old=p
    ho,p=umc(hdl,p)
    # 试各编码在 p 处
    b=hdl
    # RC
    rc=b[p]; p_rc=p+1
    # RS_LE / RS_BE
    rsle=b[p]|(b[p+1]<<8); rsbe=(b[p]<<8)|b[p+1]
    # RL LE
    rl=b[p]|(b[p+1]<<8)|(b[p+2]<<16)|(b[p+3]<<24)
    print(f"#{n} ho={ho} @{p} next={b[p:p+4].hex(' ')} RC={rc} RS_LE={rsle} RS_BE={rsbe} RL={rl} expect={exp[n]}")
    p=old+2  # 占位，下一轮不连续
