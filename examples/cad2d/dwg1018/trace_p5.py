# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress, DecompressError
import io, contextlib

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:AcDbObjects"]
pg = s.pages[1]  # page 5, off 29696
payload, hdr = c.read_data_page(pg["page_id"])
buf = bytearray(0x7400)
log = io.StringIO()
try:
    with contextlib.redirect_stdout(log):
        decompress(payload, 0x7400, 0, out=buf, out_pos=0, trace=True)
except DecompressError as e:
    print("ERR", e)
lines = log.getvalue().splitlines()
# 找 out_pos 28254 附近
for i, ln in enumerate(lines):
    if "dec=" in ln:
        try:
            dec = int(ln.split("dec=")[1].split()[0])
        except Exception:
            continue
        if 28180 <= dec <= 28270:
            print(ln)
