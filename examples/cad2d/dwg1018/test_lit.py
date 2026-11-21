# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from compress import _State, _read_literal_length
src = bytes([0x00,0x74,0x07,0xf1])
st = _State(src, 100, 0)
n = _read_literal_length(st, 0x00)
print("lit n=", n, "pos=", st.pos, "next byte=", hex(st.src[st.pos]))
