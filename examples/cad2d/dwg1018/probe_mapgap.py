import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream

hdl = open(r"G:\PyArx\examples\cad2d\dwg1018\hdl_full.bin", "rb").read()
b = BitStream(hdl)
mp = {}
pages = 0
while True:
    start = b.byte
    ss = b.read_rs_be()
    if ss > 2040: break
    last_h = 0; last_off = 0
    end = start + ss
    while b.byte < end:
        old = b.byte
        ho = b.read_umc()
        if not ho: b.byte = old+1; ho = b.read_umc()
        off = b.read_mc()
        last_h += ho; last_off += off
        mp[last_h] = last_off
    b.read_rs_be()
    pages += 1
    if b.byte >= len(hdl)-2: break

print("pages", pages, "entries", len(mp), "max handle", max(mp))
for h in [278385, 278386, 278387, 278405, 278413, 278420]:
    print(h, h in mp, mp.get(h))
# 映射表 handle 连续段缺口
ks = sorted(mp)
gaps = [(ks[i], ks[i+1]) for i in range(len(ks)-1) if ks[i+1]-ks[i] > 1]
print("num gaps", len(gaps), "largest", sorted(gaps, key=lambda g: g[1]-g[0], reverse=True)[:10])
