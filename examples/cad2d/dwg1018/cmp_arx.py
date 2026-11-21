# -*- coding: utf-8 -*-
"""按 handle 对齐：离线纯 Python 解析包 vs ARX(AcDbDatabase 官方解码) 权威值逐条比对。

权威基线：arx_lines.tsv（accoreconsole 无头经 PyArxCore.readDwgFile/遍历导出）。
定位辅助：ref_map.txt 的 handle->address（离线对象主流起点）。
本轮比对字段：handle、几何 10/20/11/21、颜色 62；层/线型/线宽/比例/可见性离线尚未解码，仅统计 ARX 侧分布。
"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin", "rb").read()


def read_h(b):
    c = b.read_rc(); sz = c & 0xf; v = 0
    for _ in range(sz):
        v = (v << 8) | b.read_rc()
    return c, sz, v


def decode_eed(b):
    end = None; q = None
    while True:
        size = b.read_bs()           # C 4079 外层顶读 BS（含结束 size=0）
        if size == 0 or size > 1000:
            break
        read_h(b)
        end = b.byte + size          # C 4123: read_H 后 byte + size
        q = BitStream(data); q.byte = b.byte; q.bit = b.bit
        while q.byte < end:
            c = q.read_rc()
            if c == 70: q.read_rs()
            elif c == 71: q.read_rl()
            elif c in (3, 5): q.read_rll_be()
            elif c == 1: q.read_rs()
            elif c == 2: q.read_rc()
            elif c == 0:
                l = q.read_rc(); q.read_rs_be(); q.read_tf(l)
            elif c == 4:
                l = q.read_rc(); q.read_tf(l)
            elif c in (10, 11, 12, 13, 14, 15):
                for _ in range(3): q.read_rd()
            elif c in (40, 41, 42): q.read_rd()
            else:
                q.byte = end; q.bit = 0
                break
        b.byte = end; b.bit = q.bit
    # 末条 size=0 BS 已在循环顶读毕，位置即主流起点；不要再压回 end。


def decode_line(addr):
    b = BitStream(data); b.byte = addr
    b.read_bs(); b.read_rl()
    _, _, handle = read_h(b)
    decode_eed(b)
    b.read_b()          # preview_exists
    b.read_bb()         # entmode
    b.read_bl()         # num_reactors
    b.read_b()          # xdic
    raw = b.read_bs()
    flag = raw >> 8
    if flag & 0x80: b.read_bl()      # ENC 0x80 rgb (BL)
    if flag & 0x20: b.read_bl()       # ENC 0x20 alpha_raw (BL)
    if (flag & 0x41) == 0x41: b.read_tv()   # ENC 0x41 color name (TV)
    if (flag & 0x42) == 0x42: b.read_tv()   # ENC 0x42 book_name (TV)
    b.read_bd()         # ltscale
    b.read_bb()         # lt_flags
    b.read_bb()         # plotstyle
    b.read_bs()         # invisible
    b.read_rc()         # linewt
    b.read_b()          # z_is_zero
    sx = b.read_rd(); ex = b.read_dd(sx)
    sy = b.read_rd(); ey = b.read_dd(sy)
    return handle, sx, sy, ex, ey, raw & 0x1ff


# ref_map: handle -> address（离线定位用）
addr_of = {}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 9 and p[2] == "LINE":
        addr_of[int(p[0])] = int(p[1])

# ARX 权威表: handle -> dict
arx = {}
with open(r"G:\PyArx\examples\cad2d\dwg1018\arx_lines.tsv", encoding="utf-8") as f:
    hdr = f.readline().rstrip("\n").split("\t")
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) < 18:
            continue
        d = dict(zip(hdr, p))
        arx[int(d["handle"])] = d

common = [h for h in arx if h in addr_of]
n = ok_hdl = ok_geom = ok_col = ok_all = 0
missing_addr = 0
bad = []
for h in common:
    addr = addr_of[h]
    a = arx[h]
    n += 1
    try:
        ph, psx, psy, pex, pey, pcol = decode_line(addr)
        hdl_ok = ph == h
        geom_ok = (abs(psx - float(a["sx"])) < 1e-6 and abs(psy - float(a["sy"])) < 1e-6
                   and abs(pex - float(a["ex"])) < 1e-6 and abs(pey - float(a["ey"])) < 1e-6)
        col_ok = pcol == int(a["colorIndex"])
        ok_hdl += hdl_ok; ok_geom += geom_ok; ok_col += col_ok
        if hdl_ok and geom_ok and col_ok:
            ok_all += 1
        else:
            bad.append((h, "hdl" if not hdl_ok else "",
                        (round(psx, 3), round(psy, 3), round(pex, 3), round(pey, 3)),
                        (round(float(a["sx"]), 3), round(float(a["sy"]), 3),
                         round(float(a["ex"]), 3), round(float(a["ey"]), 3)),
                        "col %s/%s" % (pcol, a["colorIndex"]),
                        a["layer"], a["ltype"]))
    except Exception as e:
        bad.append((h, "ERR", str(e)))

only_arx = len(arx) - len(common)
print("ARX LINE=%d  ref_map LINE=%d  可配对=%d  (ARX 独有的句柄 %d)"
      % (len(arx), len(addr_of), len(common), only_arx))
print("  handle 一致=%d  几何一致=%d  颜色一致=%d  全字段一致=%d"
      % (ok_hdl, ok_geom, ok_col, ok_all))
if n:
    print("  一致率: handle %.1f%%  几何 %.1f%%  颜色 %.1f%%  全字段 %.1f%%"
          % (100.0 * ok_hdl / n, 100.0 * ok_geom / n, 100.0 * ok_col / n,
             100.0 * ok_all / n))
print("  不符 %d 条，样例：" % len(bad))
for x in bad[:15]:
    print("  ", x)
