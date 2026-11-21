# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream

dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
cls = c.read_section("AcDb:Classs")

SENT = bytes([0x8D,0xA1,0xC4,0xB8,0xC4,0xA9,0xF8,0xC5,0xC0,0xDC,0xF4,0x5F,
              0xE7,0xCF,0xB6,0x8A])
print("sentinel match:", cls[:16] == SENT)

bs = BitStream(cls)
bs.byte = 16
size = bs.read_rl()
print(f"after sentinel @16: RL size={size} byte={bs.byte} bit={bs.bit}")
# R2004 (<R2007): 无 hsize/bitsize; 接着 BS max_num, RC 0, RC 0, B 1
max_num = bs.read_bs()
print(f"BS max_num={max_num} byte={bs.byte} bit={bs.bit}")
c1 = bs.read_rc(); c2 = bs.read_rc(); b1 = bs.read_b()
print(f"RC1={c1} RC2={c2} B={b1} byte={bs.byte} bit={bs.bit}")
print("bytes 16..32:", cls[16:32].hex(' '))
