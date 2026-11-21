# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
def skip_eed(st):
    while True:
        size=st.read_bs()
        if not size: break
        read_h(st); st.read_tf(size)

# object @20367
a=20367; b=BitStream(data); b.byte=a
ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b); eed=b.read_bs()
print(f"ms={ms} ot={hex(ot)} rl={rl} h={hex(h)}")
skip_eed(b)
print(f"afterEED byte={b.byte} bit={b.bit} abs={b.byte}.{b.bit}")
raw=data[a:a+ms+4]
print("raw:",raw.hex(' '))
# expected doubles
for name,val in [("sx",0.888997),("sy",0.817017),("ex",0.900028),("ey",0.835484)]:
    u=struct.unpack("<Q",struct.pack("<d",val))[0]
    print(name,val,"u64=",hex(u),"lebytes=",u.to_bytes(8,'little').hex(' '))
# dump RC bytes after EED boundary
abs_byte=b.byte
print("bytes after eed boundary",abs_byte,"rel",abs_byte-a)
for i in range(abs_byte, min(abs_byte+30,a+ms)):
    print(f"  rel{i-a:3d} abs{i}: {data[i]:02X}")
