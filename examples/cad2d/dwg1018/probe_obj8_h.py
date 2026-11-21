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

for off in [0, 8]:
    b = BitStream(data); b.byte=off; b.bit=0
    ms=b.read_ms(); bs=b.read_bs()
    print(f"@{off} MS={ms} BS={bs} bit={b.bit} byte={b.byte}")
    try:
        code,size,val = read_h(b)
        print(f"   H code={code} size={size} val=0x{val:X} byte={b.byte} bit={b.bit}")
    except Exception as e:
        print("   H EXC", e)
