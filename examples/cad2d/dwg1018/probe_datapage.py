# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)

for name in ["AcDb:AuxHeader", "AcDb:eader", "AcDb:Classs"]:
    s = c.sections.get(name)
    if not s:
        continue
    print(f"== {name} id={s.section_id} pages={s.page_count} comp={s.compressed}")
    for pg in s.pages:
        pid = pg["page_id"]
        addr = c._page_addr(pid)
        hdr_raw = struct.unpack_from("<8I", c.data, addr)
        mask = 0x4164536B ^ addr
        hdr = [v ^ mask for v in hdr_raw]
        print(f"  page {pid} @0x{addr:X} data_size={pg['data_size']} "
              f"hdr={[hex(x) for x in hdr]}")
        try:
            out = c.read_data_page(pid, pg["data_size"], hdr[2], pg["start_offset"])
            print(f"    decompressed {len(out)} bytes, head={out[:24].hex()}")
        except Exception as e:
            print(f"    FAIL {type(e).__name__}: {e}")
