# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
hdl = c.read_section("AcDb:Handles")
print("hdl len", len(hdl), "head", hdl[:8].hex(' '))

bs = BitStream(hdl)
endpos = len(hdl)
last_handle = 0
last_offset = 0
count = 0
page = 0
try:
    while bs.byte < endpos:
        startpos = bs.byte
        section_size = bs.read_rs_be()
        print(f"\npage {page}: section_size={section_size} @{startpos}")
        if section_size > 2040:
            print("  >2040 abort"); break
        while bs.byte - startpos < section_size:
            old = bs.byte
            handleoff = bs.read_umc()
            offset = bs.read_mc()
            last_handle += handleoff
            last_offset += offset
            count += 1
            if count <= 20:
                print(f"  obj {count}: handle={last_handle:#x} off_delta={offset} addr={last_offset} @{old}")
        crc = bs.read_rs_be()
        print(f"  crc={crc:#06x} byte={bs.byte}")
        page += 1
        if page >= 2: break
    print("\nobjects:", count)
except Exception as e:
    print("FAIL", type(e).__name__, e, "at", bs.byte, bs.bit)
