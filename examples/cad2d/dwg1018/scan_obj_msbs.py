# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("head", data[:32].hex(' '))
for off in range(0, 24):
    b = BitStream(data); b.byte = off//8; b.bit = off%8
    try:
        ms = b.read_ms()
        bs = b.read_bs()
        ok = 1 <= bs <= 0x1F2
        print(f"@{off}.{off%8} MS={ms:5d} BS={bs:4d} {'HIT' if ok else ''}")
    except Exception as e:
        print(f"@{off} EXC {e}")
