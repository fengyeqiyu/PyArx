# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress, DecompressError
dwg=r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c=open_dwg(dwg)
for name,out in [("AcDb:Handles","hdl_full.bin"),("AcDb:AcDbObjects","obj_full.bin")]:
    s=c.sections[name]
    buf=bytearray(s.max_page_size*s.page_count)
    errs=0
    for pg in s.pages:
        payload,hdr=c.read_data_page(pg["page_id"]); off=pg["start_offset"]
        try:
            decompress(payload, len(buf), 0, out=buf, out_pos=off)
        except DecompressError as e:
            errs+=1; print(name,"page",pg["page_id"],"ERR",e)
    open(r"G:\PyArx\examples\cad2d\dwg1018\\"+out,"wb").write(bytes(buf))
    print(name,"pages",s.page_count,"errs",errs,"len",len(buf))
