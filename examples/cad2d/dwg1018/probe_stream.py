# -*- coding: utf-8 -*-
import sys,struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
addr=4; idx=0; target=None
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if ot==0x13: target=(addr,ms,rl,h,end); break
    idx+=1; addr=end

a,ms,rl,h,end=target
print(f"LINE addr={a} ms={ms} bitsize={rl} h={hex(h)} end={end}")
print(f"obj byte span 0..{ms} (CRC in last 2). bitsize={rl} bits = {rl/8:.2f} bytes")
print(f"handle stream starts at bit {rl} (byte {rl//8}.{rl%8}), object size*8={ms*8}")
# main dat position after header
b=BitStream(data); b.byte=a
b.read_ms(); b.read_bs(); b.read_rl(); read_h(b)
print(f"after H handle main pos {b.byte}.{b.bit}")
# hdl_dat positioned at bitsize
hdl=BitStream(data); hdl.byte=a; hdl.bit=0
hdl.byte=rl//8; hdl.bit=rl%8
print(f"hdl stream @ {hdl.byte}.{hdl.bit} abs {a+hdl.byte}.{hdl.bit}")
print("hdl first bytes:", data[a+rl//8:a+rl//8+16].hex(' '))
