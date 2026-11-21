# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    code=b.read_rc(); sz=code&0xf; v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return code,sz,v

def decode_eed(b, addr):
    size=b.read_bs()
    if not size: return  # 无 EED：主流从当前位继续
    read_h(b); end=b.byte+size
    while True:
        sav=b.byte
        q=BitStream(data); q.byte=b.byte; q.bit=b.bit
        while q.byte < end:
            c=q.read_rc()
            print(f"  code={c} @{q.byte-addr}.{q.bit} end={end-addr}")
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
            else: print(f"  ??code={c}"); break
        b.byte=q.byte; b.bit=q.bit
        if b.byte < end - 1:  # 派生后续条目
            size=end-b.byte+1
            end=b.byte+size
            continue
        break
    b.byte=end; b.bit=6  # 有 EED：主流起点 end 字节 bit6，preview B 后 bit7

def decode(addr, sx,sy,ex,ey):
    b=BitStream(data); b.byte=addr  # 主流链头 = MS size 之后（obj->address）
    ot=b.read_bs(); bitsize=b.read_rl()
    c=b.read_rc(); hsz=c&0xf; hv=0
    for _ in range(hsz): hv=(hv<<8)|b.read_rc()
    print(f"\nh=0x{hv:X} type=0x{ot:X} bitsize={bitsize}")
    decode_eed(b, addr)
    print(f"  [main] after EED {b.byte-addr}.{b.bit}")
    preview=b.read_b(); print(f"  [main] preview={preview} @{b.byte-addr}.{b.bit}")
    entmode=b.read_bb(); print(f"  [main] entmode @{b.byte-addr}.{b.bit} v={entmode}")
    numr=b.read_bl(); print(f"  [main] numr @{b.byte-addr}.{b.bit} v={numr}")
    xdic=b.read_b(); print(f"  [main] xdic @{b.byte-addr}.{b.bit} v={xdic}")
    raw=b.read_bs(); print(f"  [main] color @{b.byte-addr}.{b.bit} v=0x{raw:X}")
    ltscale=b.read_bd(); print(f"  [main] ltscale @{b.byte-addr}.{b.bit} v={ltscale}")
    ltype_flags=b.read_bb(); print(f"  [main] lt_flags @{b.byte-addr}.{b.bit} v={ltype_flags}")
    plotstyle=b.read_bb(); print(f"  [main] plotstyle @{b.byte-addr}.{b.bit} v={plotstyle}")
    invisible=b.read_bs(); print(f"  [main] inv @{b.byte-addr}.{b.bit} v={invisible}")
    linewt=b.read_rc(); print(f"  [main] linewt @{b.byte-addr}.{b.bit} v=0x{linewt:X}")
    print(f"  entmode={entmode} numr={numr} xdic={xdic}")
    print(f"  color.raw=0x{raw:X} idx={raw&0x1ff} ltscale={ltscale} lt_flags={ltype_flags} plotstyle={plotstyle} inv={invisible} linewt=0x{linewt:X}")
    zzero=b.read_b()
    sx_=b.read_rd()
    ex_=b.read_dd(sx_)
    sy_=b.read_rd()
    ey_=b.read_dd(sy_)
    print(f"  zzero={zzero} start=({sx_},{sy_}) end=({ex_},{ey_})")
    print(f"  expect=({sx},{sy})-({ex},{ey})")

decode(20374376,529687.5,300.0,527812.5,300.0)
