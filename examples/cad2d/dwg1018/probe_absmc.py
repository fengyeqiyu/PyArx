# -*- coding: utf-8 -*-
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
def mc_enc(v):
    neg=v<0; v=abs(v); b=[0]*5; j=0
    for i in range(5): b[i]=(v>>j)&0x7f; j+=7
    top=4
    while top>0 and not b[top]: top-=1
    if b[top]&0x40 and top>0: top-=1
    for i in range(top): b[i]|=0x80
    if neg: b[top]|=0x40
    return bytes(reversed(b[:top+1]))
for loc in [2890,7614,98837,43053,363755,134618]:
    e=mc_enc(loc)
    print(loc,"mc",e.hex(),"@",hdl.find(e))
# also search raw BE/LE 16-bit/32-bit of loc
import struct
for loc in [2890,7614]:
    print(loc,"BE16",struct.pack(">H",loc).hex(),"@",hdl.find(struct.pack(">H",loc)),
          "LE16",struct.pack("<H",loc).hex(),"@",hdl.find(struct.pack("<H",loc)),
          "BE32",struct.pack(">I",loc).hex(),"@",hdl.find(struct.pack(">I",loc)))
