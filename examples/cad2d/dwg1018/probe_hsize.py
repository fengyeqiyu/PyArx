# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
print("len",len(hdl),"declared size 637399")
print("around 637399:",hdl[637390:637410].hex(' '))
# 找最后一个非零字节
nz=max(i for i,b in enumerate(hdl) if b)
print("last nonzero",nz)
# 22 数据页每页 29696；句柄表小节连续跨页。统计前 637399 内 section_size 链
p=0; pg=0
while p+2<=637399:
    ss=(hdl[p]<<8)|hdl[p+1]
    if ss>2040:
        print(f"BAD ss={ss} @{p}"); break
    end=p+ss
    if end>637399: print(f"page {pg}: ss={ss} end={end} >size"); break
    p=end+2; pg+=1
print("sections consumed",pg,"end",p)
