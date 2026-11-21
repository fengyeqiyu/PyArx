# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
from compress import decompress, DecompressError
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
s = c.sections["AcDb:Handles"]
pg = s.pages[0]
payload, hdr = c.read_data_page(pg["page_id"])
print("data", hdr["data_size"], "page", hdr["page_size"], "off", hdr["start_offset"])
buf = bytearray(s.max_page_size * s.page_count)
# 手动跑主循环插桩
import compress
st = compress._State(payload, len(payload), 0, out=buf, out_pos=0)
op1 = st.next()
lit = compress._read_literal_length(st, op1)
op1 = st.copy_bytes(lit)
print(f"start lit={lit} src={st.pos} dec={st.out_pos}")
n=0
try:
    while st.pos < st.end and st.out_pos < len(buf) and op1 != 0x11:
        cb=off=litc=0
        if op1 < 0x10 or op1 >= 0x40:
            cb=(op1>>4)-1; op2=st.next(); off=(((op1>>2)&3)|(op2<<2))+1; litc=op1&3
        elif 0x12<=op1<=0x1F:
            cb=compress._compressed_bytes(st,op1,7); high=(op1&8)<<11; off,litc=compress._two_byte_offset(st,0x4000,high)
        else:
            cb=compress._compressed_bytes(st,op1,0x1F); off,litc=compress._two_byte_offset(st,1)
        print(f"op=0x{op1:02X} src={st.pos} dec={st.out_pos} cb={cb} off={off} lit={litc}")
        n+=1
        start=st.out_pos-off
        if start<0:
            print(f"INVALID off={off} dec={st.out_pos}"); break
        st.emit_match(cb,off)
        if litc==0:
            op1=st.next()
            if (op1&0xF0)==0: litc=compress._read_literal_length(st,op1)
        if litc:
            op1=st.copy_bytes(litc)
except Exception as e:
    print("EXC", e)
print("ops", n, "dec", st.out_pos, "src", st.pos)
