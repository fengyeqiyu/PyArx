# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
sm = c.read_system_page(0x4163003B)
num = struct.unpack_from("<I", sm, 0)[0]
print("num", num, "sm len", len(sm))
pos = 0x14
for i in range(num):
    size, pc, maxp, unk, comp, sid, enc = struct.unpack_from("<QIIIIII", sm, pos)
    name = sm[pos+0x20:pos+0x60].split(b"\x00",1)[0].decode("gbk","replace")
    print(f"[{i}] size={size} pages={pc} maxp={maxp} unk={unk} comp={comp} id={sid} enc={enc} name={name!r}")
    pos += 0x20 + 0x40
    for _ in range(pc):
        pn, ds, so = struct.unpack_from("<IIQ", sm, pos)
        pos += 0x10
