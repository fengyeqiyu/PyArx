# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin","rb").read()
# 2802 各编码尝试
v=2802
enc_mc = bytes([(v&0x7F)|0x80, (v>>7)&0x7F])  # F2 15
enc_rs = v.to_bytes(2,'little')
enc_rsbe=v.to_bytes(2,'big')
enc_rl = v.to_bytes(4,'little')
for name,pat in [("MC",enc_mc),("RS_LE",enc_rs),("RS_BE",enc_rsbe),("RL",enc_rl)]:
    hits=[i for i in range(0,3000) if data[i:i+len(pat)]==pat]
    print(name, pat.hex(), hits[:8])
