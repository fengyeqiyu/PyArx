# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
b=data[4:40]
print("obj0 bytes:",b.hex(' '))
# MS: first RS LE
def rs(p): return data[p]|(data[p+1]<<8)
ms=rs(4); print("MS @4:",ms, "raw",data[4:6].hex())
# BS at 6
print("BS @6:",hex(data[6]))
# RL at 7: two RS
w1=rs(7); w2=rs(9); rl=(w2<<16)|w1
print("RL @7:",rl,"raw",data[7:11].hex())
# H at 11
print("H code @11:",hex(data[11]),"bytes",data[12:13].hex())
