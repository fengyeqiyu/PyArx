# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
exec(open(r"G:\PyArx\examples\cad2d\dwg1018\probe_objcrc.py").read().split("addr=4")[0])
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=4
# 扫描 size 2700..2900, CRC 覆盖 addr..addr+size-2, 读 RS(LE)
for size in range(2700, 2900):
    end=addr+size
    calc=crc16(data[addr:end-2])
    read=data[end-2]|(data[end-1]<<8)
    if calc==read:
        print(f"MATCH size={size} end={end} crc={calc:04X}")
print("done scan")
