# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
# 权威 ref: handle -> address(=体起点+2, MS后位置)
ref={}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"):
    h,a,t,n=line.rstrip("\n").split("\t",3)
    ref[int(h,16)]=int(a)
b=BitStream(hdl)
# 读句柄表页0
ss=b.read_rs_be(); start=b.byte; last_h=0; last_off=0
n=0; bad=0
while b.byte-start < ss:
    ho=b.read_umc()
    if not ho: b.byte+=1; ho=b.read_umc()
    off=b.read_mc()
    last_h+=ho; last_off+=off
    body=last_off  # 体起点
    ref_a=ref.get(last_h)
    if ref_a is not None:
        # ref_a = body + ms_read;  body = ref_a - 2? 核对
        if n<15:
            print(f"#{n} h=0x{last_h:X} body={body} ref_addr={ref_a} ref-2={ref_a-2} match={body==ref_a-2}")
    n+=1
print("entries",n)
