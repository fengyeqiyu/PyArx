# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
hdl=open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
# page0: RS_BE size @0, entries follow at 2; CRC at end
import struct
ss=struct.unpack_from(">H",hdl,0)[0]
print("section_size",ss,"data end",2+ss)
print("entry bytes:",hdl[2:2+40].hex(' '))
print("CRC @",2+ss, hdl[2+ss:2+ss+2].hex(' '))
# Known object chain sizes: obj0 size2798 -> loc 4; obj1 loc 2806 (MC=2802); obj2 loc2966 (MC=160)
# decode MC manually at bytes after first entry (4)
def mc(b,p):
    neg=0;v=0;k=0
    for _ in range(5):
        x=b[p];p+=1
        if not x&0x80:
            if x&0x40: neg=1;x&=0xbf
            return (-v if neg else v),p
        v|=(x&0x7f)<<k;k+=7
    return 0,p
p=2
for i in range(5):
    # UMC
    ho=0;k=0
    for _ in range(8):
        x=hdl[p];p+=1
        if not x&0x80: ho|=x<<k; break
        ho|=(x&0x7f)<<k;k+=7
    off,p=mc(hdl,p)
    print(i,"ho",ho,"mc",off,"p",p)
