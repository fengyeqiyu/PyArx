# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
# BS 编码：BB=0 RS(2字节小端); BB=1 RC(1字节); BB=2 ->0; BB=3 ->256
# 类型 1..20。BS=1: BB=1 后跟 0x01 (RC) => 10 01 ; BB=0 RS=00 01 => 00 01?
# 直接扫描前 256 字节，尝试在每个字节起点读 MS 然后 BS，看 BS 落在 1..31 或 256(类表)
from bitio import BitStream
for off in range(0, 128):
    b = BitStream(data); b.byte = off; b.bit = 0
    try:
        ms = b.read_ms()
        if ms <= 0 or ms > 2000: continue
        bs = b.read_bs()
        if 1 <= bs <= 31 or bs == 256:
            print(f"byte@{off} MS={ms} BS={bs} bit={b.bit}")
    except Exception:
        pass
