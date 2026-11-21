# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from bitio import BitStream
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
obj = c.read_section("AcDb:AcDbObjects")

# MS@0 = word1=0x0dca, highbit 0 -> 单字, 占 2 字节, 游标到 byte2 bit0
# BS 从 byte2=00 bit0: BB=00 -> RS 读两字节 00 00 = 0
bs = BitStream(obj); bs.byte=0
print("MS", bs.read_ms(), "cursor", bs.byte, bs.bit)
print("BS", bs.read_bs(), "cursor", bs.byte, bs.bit)
# type=0 是 UNUSED。试 type 字段为 BOT(2010) 不适用。
# 若首字段其实是 RL(4字节 0x0dca)，type 从 byte4
bs = BitStream(obj); bs.byte=4
print("from @4 BS", bs.read_bs(), "cursor", bs.byte, bs.bit)
