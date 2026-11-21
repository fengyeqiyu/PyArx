# -*- coding: utf-8 -*-
import struct
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
obj=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
targets={0x1:4,0x4237a:2806,0x423d5:2966,0x423d9:3033}
for h,a in targets.items():
    for fmt,fsz in ("<I",4),("<Q",8),("<H",2),("I",4):
        pat=struct.pack(fmt,a)
        idx=[];p=0
        while True:
            p=hdl.find(pat,p)
            if p<0: break
            idx.append(p);p+=1
        if idx: print(f"h={h:#x} addr={a} {fmt}({fsz}B) hits {idx[:4]}")
# 对象节里搜 h=0x4237a 编码（RC 长度1? 看头）
