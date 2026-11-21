# -*- coding: utf-8 -*-
"""容错解压 PM，越界即停，导出部分输出并按 8 字节条目解析。"""
import struct
import sys

sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import compress

p = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
d = open(p, "rb").read()
hit = d.find(struct.pack("<I", 0x41630E3B))
t, ds, cs, ct, ck = struct.unpack_from("<LLLLL", d, hit)
body = d[hit + 0x14: hit + 0x14 + cs]

st = compress._State(body, ds, hit + 0x14)
op1 = st.next()
lit = compress._read_literal_length(st, op1)
op1 = st.copy_bytes(lit)

rows = []
try:
    while st.pos < st.end and len(st.out) < st.limit and op1 != 0x11:
        comp_bytes = 0
        comp_offset = 0
        lit_count = 0
        if op1 == 0x10:
            comp_bytes = compress._long_offset(st) + 9
            comp_offset, lit_count = compress._two_byte_offset(st, 0x3FFF)
        elif op1 == 0x20:
            comp_bytes = compress._long_offset(st) + 0x21
            comp_offset, lit_count = compress._two_byte_offset(st, 0)
        elif 0x12 <= op1 <= 0x1F:
            comp_bytes = (op1 & 0x0F) + 2
            comp_offset, lit_count = compress._two_byte_offset(st, 0x3FFF)
        elif 0x21 <= op1 <= 0x3F:
            comp_bytes = op1 - 0x1E
            comp_offset, lit_count = compress._two_byte_offset(st, 0)
        else:
            comp_bytes = ((op1 & 0xF0) >> 4) - 1
            op2 = st.next()
            comp_offset = (op2 << 2) | ((op1 & 0x0C) >> 2)
            lit_count = op1 & 0x03

        if comp_offset > len(st.out):
            print(f"STOP off={comp_offset} out={len(st.out)} op={op1:02X} len={comp_bytes}")
            break
        rows.append((len(st.out), op1, comp_bytes, comp_offset, lit_count))
        st.emit_match(comp_bytes, comp_offset)
        if lit_count == 0:
            op1 = st.next()
            if (op1 & 0xF0) == 0:
                lit_count = compress._read_literal_length(st, op1)
        if lit_count:
            op1 = st.copy_bytes(lit_count)
        else:
            pass
except Exception as e:
    print("EXC", e)

out = bytes(st.out)
print("partial out len", len(out))
for i in range(0, min(len(out), 96), 8):
    pid, sz = struct.unpack_from("<iI", out, i)
    print(f"  [{i//8:3d}] id={pid:6d} size={sz:6d}")
print("last 8 rows:", rows[-8:])
open(r"G:\PyArx\examples\cad2d\dwg1018\pm_partial.bin", "wb").write(out)
