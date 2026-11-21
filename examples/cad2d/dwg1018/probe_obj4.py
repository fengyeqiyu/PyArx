# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(st):
    code = st.read_rc()
    size = code & 0xF; code_hi = (code>>4)&0xF
    val = 0
    for i in range(size):
        val = (val<<8) | st.read_rc()
    return code_hi, size, val

b = BitStream(data)
# 节级前置 RL
pre = b.read_rl()
print("section RL pre =", pre, "byte", b.byte)
ms=b.read_ms(); ot=b.read_bs(); rl=b.read_rl()
print(f"@4 MS={ms} OT={ot} RL bitsize={rl} byte={b.byte} bit={b.bit}")
code,size,val=read_h(b)
print(f"  H code={code} size={size} val=0x{val:X} byte={b.byte} bit={b.bit}")
eed=b.read_bs()
print(f"  EED size={eed} byte={b.byte} bit={b.bit}")
