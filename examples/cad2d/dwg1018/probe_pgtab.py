# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
c=open_dwg(r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg")
for name in ["AcDb:Handles","AcDb:AcDbObjects"]:
    s=c.sections[name]
    print("==",name,"pages",s.page_count,"max",s.max_page_size,"size",s.size)
    prev=None
    for pg in s.pages:
        off=pg["start_offset"]
        gap="" if prev is None else f"delta={off-prev}"
        print(f"  pid={pg['page_id']} off={off} {gap}")
        prev=off
# page map raw
pm=c.read_system_page(0x41630E3B)
print("pm len",len(pm))
pos=0; rows=[]
while pos+8<=len(pm):
    pid,sz=struct.unpack_from("<ii",pm,pos); pos+=8
    if pid<0:
        parent,left,right,z=struct.unpack_from("<iiii",pm,pos); pos+=16
        rows.append((pid,sz,parent,left,right))
    else:
        rows.append((pid,sz))
print("pm rows",len(rows),"neg",sum(1 for r in rows if r[0]<0))
