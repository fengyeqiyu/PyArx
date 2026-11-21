# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)

# 先验证整节拼接：Handles
hdl = c.read_section("AcDb:Handles")
print("handles buffer:", len(hdl), "head:", hdl[:16].hex())

# 对照源码 2583：每页先 RS_BE section_size，再循环 (UMC handleoff, MC offset)，末尾 RS_BE CRC
bs = BitStream(hdl)
endpos = len(hdl)
last_handle = 0
last_offset = 0
prevsize = 0
count = 0
page = 0
try:
    while bs.byte < endpos:
        startpos = bs.byte
        section_size = bs.read_rs_be()
        print(f"\npage {page}: section_size={section_size} @{startpos}")
        if section_size > 2040:
            print("  >2040, abort"); break
        while bs.byte - startpos < section_size:
            old = bs.byte
            handleoff = bs.read_umc()
            offset = bs.read_mc()
            last_handle += handleoff
            last_offset += offset
            count += 1
            if count <= 15:
                print(f"  obj {count}: handle={last_handle:#x} handleoff={handleoff} "
                      f"offset={offset} addr={last_offset} @{old}")
            prevsize = 0  # 无对象尺寸信息，跳过校验
        crc = bs.read_rs_be()
        print(f"  crc={crc:#06x} byte={bs.byte}")
        page += 1
        if page >= 3:
            break
    print("\nparsed objects:", count)
except Exception as e:
    print("FAIL", type(e).__name__, e, "at", bs.byte)
