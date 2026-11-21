# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream
for path in [r"G:\PyArx\docs\libredwg_src\test\test-data\2004\circle.dwg"]:
    try:
        c=open_dwg(path)
        print("sections:",[(k,v.page_count,v.max_page_size) for k,v in c.sections.items()])
        hdls=[k for k in c.sections if "Handle" in k]
        if not hdls: continue
        hb=c.read_section(hdls[0])
        print("hdl buf len",len(hb),"first 12:",hb[:12].hex(' '))
        st=BitStream(hb)
        ss=st.read_rs_be(); print("size",ss)
        lo=0;lh=0
        for i in range(10):
            ho=st.read_umc();off=st.read_mc();lo+=off;lh+=ho
            print(i,"h",lh,"ho",ho,"off",off,"loc",lo)
    except Exception as e:
        print("ERR",type(e).__name__,e)
