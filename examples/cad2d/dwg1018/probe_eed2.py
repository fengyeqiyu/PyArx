# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
addr=140531; body=addr-2
b=BitStream(data); b.byte=body
b.read_ms(); b.read_bs(); b.read_rl()
c=b.read_rc(); hsz=c&0xf
for _ in range(hsz): b.read_rc()
print(f"after header rel(addr)={b.byte-addr}.{b.bit}")
size=b.read_bs()
print(f"EED size={size} @{b.byte-addr}.{b.bit}")
code=b.read_rc(); sz=code&0xf; hv=0
for _ in range(sz): hv=(hv<<8)|b.read_rc()
print(f"  hcode={code} hsz={sz} h=0x{hv:X} @{b.byte-addr}.{b.bit}")
end=b.byte+size
print(f"  end={end-addr}")
# 数据解码副本（保留位位置）
q=BitStream(data); q.byte=b.byte; q.bit=b.bit
while q.byte < end:
    cc=q.read_rc()
    if cc==70:
        v=q.read_rs(); print(f"  data 70 rs={v} @{q.byte-addr}.{q.bit}")
    elif cc==71:
        v=q.read_rl(); print(f"  data 71 rl={v} @{q.byte-addr}.{q.bit}")
    elif cc in (3,5):
        v=q.read_rll_be(); print(f"  data {cc} rllbe=0x{v:X} @{q.byte-addr}.{q.bit}")
    elif cc==1: q.read_rs()
    elif cc==2: q.read_rc()
    elif cc==0:
        l=q.read_rc(); q.read_tf(l)
    elif cc==4:
        l=q.read_rc(); q.read_tf(l)
    elif cc in (10,11,12,13,14,15):
        for _ in range(3): q.read_rd()
    elif cc in (40,41,42): q.read_rd()
    else:
        print(f"  ??code={cc}"); break
print(f"  data end @{q.byte-addr}.{q.bit}")
b.byte=end; b.bit=0
print(f"main after eed rel={b.byte-addr}.{b.bit}")
