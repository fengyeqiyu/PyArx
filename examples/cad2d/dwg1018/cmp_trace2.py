# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress
dwg=r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c=open_dwg(dwg); s=c.sections["AcDb:Handles"]; pg=s.pages[0]
payload,hdr=c.read_data_page(pg["page_id"])
import io,contextlib
buf=io.StringIO()
try:
    with contextlib.redirect_stdout(buf):
        decompress(payload, hdr["page_size"], trace=True)
except Exception as e:
    print("EXC", type(e).__name__, e)
lines=[l for l in buf.getvalue().splitlines() if "PY]" in l]
open(r"G:\PyArx\examples\cad2d\dwg1018\py_trace.txt","w").write("\n".join(lines))
print("py lines", len(lines))
