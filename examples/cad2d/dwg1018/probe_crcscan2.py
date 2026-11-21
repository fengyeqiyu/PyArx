# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
exec(open(r"G:\PyArx\examples\cad2d\dwg1018\probe_objcrc.py").read().split("addr=4")[0])
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=4
for size in range(2750, 2860):
    end=addr+size
    calc=crc16(data[addr:end-2])
    read=data[end-2]|(data[end-1]<<8)
    if calc==read:
        print(f"MATCH size={size} end={end} crc={calc:04X}")
# 句柄表首条 offset=4; 若 size 字段值=2798, 下一对象地址 = 4+2798+4=2806
# CRC 边界 2802. 列出 2802/2806 处 CRC 字
for e in [2802,2806]:
    print(f"end={e} RS_LE=0x{(data[e-2]|data[e-1]<<8):04X} RS_BE=0x{(data[e-2]<<8)|data[e-1]:04X}")
