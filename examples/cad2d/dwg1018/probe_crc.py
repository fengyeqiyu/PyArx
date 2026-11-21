# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
hdl = c.read_section("AcDb:Handles")

def crc16(data, init=0xC0C1):
    crc = init
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
            crc &= 0xFFFF
    return crc

bs = BitStream(hdl)
start = bs.byte
section_size = bs.read_rs_be()
print(f"section_size={section_size} @{start}")
body_start = bs.byte
# 跳过 body，读 CRC
end_body = start + section_size
# 读条目直到 body 结束
count = 0
while bs.byte < end_body:
    old = bs.byte
    ho = bs.read_umc(); off = bs.read_mc()
    count += 1
    if bs.byte == old: break
crc_read = bs.read_rs_be()
crc_calc = crc16(hdl[start:end_body])
print(f"entries={count} crc_read={crc_read:#06x} crc_calc={crc_calc:#06x} match={crc_read==crc_calc}")
print(f"body end={end_body} after crc byte={bs.byte}")
