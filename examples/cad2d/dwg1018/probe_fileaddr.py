# -*- coding: utf-8 -*-
import struct
dwg=open(r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg","rb").read()
print("filesize",len(dwg))
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
for a in (4,2806,2966,3033,3075):
    for fmt in ("<I","<Q"):
        pat=struct.pack(fmt,a)
        idx=[];p=0
        while True:
            p=dwg.find(pat,p)
            if p<0: break
            idx.append(p);p+=1
        ih=[];p=0
        while True:
            p=hdl.find(pat,p)
            if p<0: break
            ih.append(p);p+=1
        if idx or ih: print(f"addr={a} {fmt} file={idx[:4]} hdl={ih[:4]}")
