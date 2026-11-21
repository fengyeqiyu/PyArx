# -*- coding: utf-8 -*-
"""按 handle 对齐：Python 解析包读出的值 vs C 无头 dump_hdl 权威值，逐条比对。
权威基线 ref_map.txt：handle(十进制)  addr  TYPE  sx  sy  ex  ey  layer.handle  color.index
"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    c=b.read_rc(); sz=c&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return c,sz,v

def decode_eed(b):
    size=b.read_bs()
    if not size: return
    read_h(b); end=b.byte+size
    while True:
        q=BitStream(data); q.byte=b.byte; q.bit=b.bit
        while q.byte < end:
            c=q.read_rc()
            if c==70: q.read_rs()
            elif c==71: q.read_rl()
            elif c in (3,5): q.read_rll_be()
            elif c==1: q.read_rs()
            elif c==2: q.read_rc()
            elif c==0:
                l=q.read_rc(); q.read_tf(l)
            elif c==4:
                l=q.read_rc(); q.read_tf(l)
            elif c in (10,11,12,13,14,15):
                for _ in range(3): q.read_rd()
            elif c in (40,41,42): q.read_rd()
            else: break
        b.byte=q.byte; b.bit=q.bit
        if b.byte < end - 1:
            size=end-b.byte+1; end=b.byte+size; continue
        break
    b.byte=end; b.bit=6

def decode_line(addr):
    b=BitStream(data); b.byte=addr
    b.read_bs(); b.read_rl()
    _,_,handle=read_h(b)
    decode_eed(b)
    b.read_b()          # preview_exists
    b.read_bb()         # entmode
    b.read_bl()         # num_reactors
    b.read_b()          # xdic
    raw=b.read_bs()
    b.read_bd()         # ltscale
    b.read_bb()         # lt_flags
    b.read_bb()         # plotstyle
    b.read_bs()         # invisible
    b.read_rc()         # linewt
    b.read_b()          # z_is_zero
    sx=b.read_rd(); ex=b.read_dd(sx); sy=b.read_rd(); ey=b.read_dd(sy)
    return handle, sx,sy,ex,ey,raw&0x1ff

# 权威表：handle -> (addr, sx,sy,ex,ey, layer, color)
auth={}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt",encoding="utf-8"):
    p=line.rstrip("\n").split("\t")
    if len(p)>=9 and p[2]=="LINE":
        auth[int(p[0])]=(int(p[1]), float(p[3]),float(p[4]),float(p[5]),float(p[6]),
                          int(p[7]), int(p[8]))

n=ok_hdl=ok_geom=ok_col=ok_all=0
bad=[]
for h,(addr,esx,esy,eex,eey,elay,ecol) in auth.items():
    n+=1
    try:
        ph,psx,psy,pex,pey,pcol=decode_line(addr)
        hdl_ok = ph==h
        geom_ok=abs(psx-esx)<1e-6 and abs(psy-esy)<1e-6 and abs(pex-eex)<1e-6 and abs(pey-eey)<1e-6
        col_ok = pcol==ecol
        ok_hdl+=hdl_ok; ok_geom+=geom_ok; ok_col+=col_ok
        if hdl_ok and geom_ok and col_ok: ok_all+=1
        else: bad.append((h, "hdl" if not hdl_ok else "",
                          (round(psx,3),round(psy,3),round(pex,3),round(pey,3)),
                          (esx,esy,eex,eey), f"col {pcol}/{ecol}"))
    except Exception as e:
        bad.append((h,"ERR",str(e))); n-=0

print(f"LINE 总数={n}")
print(f"  handle 一致={ok_hdl}  几何一致={ok_geom}  颜色一致={ok_col}  全字段一致={ok_all}")
print(f"  不符 {len(bad)} 条，样例：")
for x in bad[:12]: print("  ", x)
