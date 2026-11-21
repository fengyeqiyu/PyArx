# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")
print("bytes 0..10:", obj[:11].hex(' '))
bs = BitStream(obj); bs.byte=0
print("MS", bs.read_ms(), "cursor", bs.byte, bs.bit)
print("BB", bs.read_bb(), "cursor", bs.byte, bs.bit)
print("RC1", hex(bs.read_rc()), "cursor", bs.byte, bs.bit)
print("RC2", hex(bs.read_rc()), "cursor", bs.byte, bs.bit)
