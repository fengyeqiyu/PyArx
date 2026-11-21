# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import compress
payload = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_body.bin","rb").read()

orig = compress._two_byte_offset
def tb(st, plus, high=0):
    off, lit = orig(st, plus, high)
    return off, lit

# 直接调内部循环，插桩 next
st = compress._State(payload, len(payload), 11328)
import types
out = bytearray(11328)
st.out = out
st.out_pos = 0
n=0
while True:
    if st.out_pos >= len(out): break
    if st.pos >= st.end: break
    op = st.next()
    if op < 0x10:  # literal length
        cnt = op
        if op == 0:
            cnt = compress._read_literal_length(st)
        elif op < 4:
            cnt += compress._read_literal_length(st)
        for i in range(cnt):
            out[st.out_pos] = st.next(); st.out_pos+=1
        print(f"[PY] lit n={cnt} src={st.pos} dec={st.out_pos}")
        continue
    if op == 0x10:
        cb = compress._long_offset(st)+9
        off, lit = compress._two_byte_offset(st, 0x3FFF)
    elif op == 0x20:
        cb = compress._long_offset(st)+0x21
        off, lit = compress._two_byte_offset(st, 0)
    elif 0x12 <= op <= 0x1F:
        cb = compress._compressed_bytes(st, op, 7)
        high=(op&8)<<11
        off, lit = compress._two_byte_offset(st, 0x4000, high)
    elif 0x21 <= op <= 0x3F:
        cb = compress._compressed_bytes(st, op, 0x1F)
        off, lit = compress._two_byte_offset(st, 1)
    else:
        cb=(op>>4)-1; op2=st.next(); off=((op&0x0F)<<8)|op2; lit=0
    n+=1
    print(f"[PY] op=0x{op:02X} src={st.pos} dec={st.out_pos} cb={cb} off={off} lit={lit}")
    if off==0 or off>st.out_pos:
        print("INVALID"); break
    for i in range(cb):
        out[st.out_pos]=out[st.out_pos-off]; st.out_pos+=1
    for i in range(lit):
        out[st.out_pos]=st.next(); st.out_pos+=1
    if n>=16: break
