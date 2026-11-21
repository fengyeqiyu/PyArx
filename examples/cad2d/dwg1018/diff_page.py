# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress, DecompressError

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:AcDbObjects"]
auth = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_auth.bin", "rb").read()

P = 0x7400
for pg in s.pages[:3]:
    payload, hdr = c.read_data_page(pg["page_id"])
    off = pg["start_offset"]
    buf = bytearray(P)
    try:
        decompress(payload, P, 0, out=buf, out_pos=0)
    except DecompressError as e:
        print("page", pg["page_id"], "ERR", e)
        continue
    a = auth[off:off+P]
    first = -1
    for i in range(P):
        if buf[i] != a[i]:
            first = i
            break
    print("page", pg["page_id"], "off", off, "comp", hdr["data_size"],
          "decomp", hdr["page_size"], "first diff in-page", first)
    if first >= 0:
        print("  auth  :", " ".join("%02x" % b for b in a[first-4:first+12]))
        print("  offlin:", " ".join("%02x" % b for b in buf[first-4:first+12]))
