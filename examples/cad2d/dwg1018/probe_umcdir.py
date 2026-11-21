# -*- coding: utf-8 -*-
def enc(v):
    byte=[0]*8
    for i in range(7,-1,-1):
        byte[i]=v&0x7f; v>>=7
    nz=next(i for i in range(8) if byte[i]&0x7f)
    # write 倒序出（高位槽先）
    return [x|0x80 for x in reversed(byte[nz:])], byte[nz:]
for v in (1,704,0x57F-0x2BF):
    hi,lo=enc(v)
    print(v,"high-first",[hex(x) for x in hi],"low-first",[hex(x|0x80) for x in lo])
