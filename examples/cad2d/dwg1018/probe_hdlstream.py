# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
def read_h(st):
    code=st.read_rc(); size=code&0xF; v=0
    for _ in range(size): v=(v<<8)|st.read_rc()
    return v
addr=4; idx=0; target=[]
while addr < len(data)-4:
    b=BitStream(data); b.byte=addr
    ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl(); h=read_h(b)
    if not(0<ms<0x40000 and 0<=ot<0x1f4 and 0<h<0x100000): break
    end=addr+ms+4
    if ot==0x13: target.append((addr,ms,rl,h,end))
    idx+=1; addr=end

for (a,ms,rl,h,end) in target[:3]:
    hdl=BitStream(data); hdl.byte=a; hdl.bit=0
    hdl.byte=rl//8; hdl.bit=rl%8
    print(f"LINE @{a} ms={ms} bitsize={rl} h={hex(h)} hdl@{hdl.byte}.{hdl.bit}")
    # common_entity_handle_data: entmode==0 -> owner H; reactors BL; xdic H; layer H; ltype H(cond)
    entmode=None
    # owner only if entmode==0, but entmode is on main dat (common_entity_data).
    # Just dump sequential handles from hdl stream
    for k in range(5):
        p=hdl.tell()
        try:
            hh=read_h(hdl)
            print(f"   hdl[{k}] @bit{p} = {hex(hh)}")
        except Exception as e:
            print(f"   hdl[{k}] ERR {e}"); break
