# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from compress import decompress, DecompressError
payload = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_body.bin","rb").read()
ref = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_ref.bin","rb").read()
print("payload", len(payload), "ref", len(ref))
buf = bytearray(11328)
try:
    decompress(payload, 11328, 0, out=buf, out_pos=0)
except DecompressError as e:
    print("PY FAIL", e)
# 逐字节对比
mism = next((i for i in range(min(len(buf),len(ref))) if buf[i]!=ref[i]), None)
print("first mismatch:", mism)
if mism is not None:
    print("py  :", bytes(buf[max(0,mism-8):mism+16]).hex(' '))
    print("ref :", bytes(ref[max(0,mism-8):mism+16]).hex(' '))
