# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress
dwg=r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c=open_dwg(dwg)
s=c.sections["AcDb:Handles"]
pg=s.pages[0]
payload,hdr=c.read_data_page(pg["page_id"])
print("data_size",hdr["data_size"],"page_size",hdr["page_size"])
out=decompress(payload, hdr["page_size"], trace=True)
ref=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_ref.bin","rb").read()
diff=[i for i in range(min(len(out),len(ref))) if out[i]!=ref[i]]
print("first diff", diff[:5], "count", len(diff))
