# -*- coding: utf-8 -*-
import sys, struct
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()

def read_h(b):
    code=b.read_rc(); sz=code&0xf
    v=0
    for _ in range(sz): v=(v<<8)|b.read_rc()
    return code,sz,v

# EED code 编码（R2004），返回消费后位置（dat 被复位到 sav_byte 再跳到 end）
EED_NEED={0:1,1:2,2:1,3:8,4:1,5:8,70:2,71:4,40:8,41:8,42:8}
def eed_data_len(code):
    if code in (10,11,12,13,14,15): return 24
    if code in (40,41,42): return 8
    if code in (70,): return 2
    if code in (71,): return 4
    if code in (3,5): return 8
    if code==1: return 2
    if code==2: return 1
    if code==4: return 1
    if code==0: return None  # 变长
    return 1

def decode_eed(b, objsize):
    idx=0
    while True:
        size=b.read_bs()
        if not size: break
        code,hsz,hv=read_h(b)
        end=b.byte+size
        sav=b.byte
        consumed=0
        while b.byte < end:
            c=b.read_rc()
            if c==0:
                lenc=b.read_rc(); b.read_tf(lenc); consumed=2+lenc
            elif c==1:
                b.read_rs(); consumed=2
            elif c==2:
                b.read_rc(); consumed=1
            elif c in (3,5):
                b.read_rll_be(); consumed=8
            elif c in (10,11,12,13,14,15):
                for _ in range(3): b.read_rd(); consumed=24
            elif c in (40,41,42):
                b.read_rd(); consumed=8
            elif c==70:
                b.read_rs(); consumed=2
            elif c==71:
                b.read_rl(); consumed=4
            elif c==4:
                l=b.read_rc(); b.read_tf(l); consumed=1+l
            else:
                print(f"  ?? eed code {c}"); break
        b.byte=end; b.bit=0
        print(f"  eed{idx} size={size} hcode={code} hsz={hsz} h=0x{hv:X}")
        idx+=1
        if idx>50: break
    return idx

def decode_line(addr, expect):
    body=addr-2
    b=BitStream(data); b.byte=body
    ms=b.read_ms(); ot=b.read_bs(); bitsize=b.read_rl()
    c=b.read_rc(); hsz=c&0xf; hv=0
    for _ in range(hsz): hv=(hv<<8)|b.read_rc()
    print(f"\nh=0x{hv:X} addr={addr} ms={ms} type=0x{ot:X} bitsize={bitsize}")
    decode_eed(b, ms)
    preview=b.read_b()
    entmode=b.read_bb()
    numr=b.read_bl()
    xdic=b.read_b()
    raw=b.read_bsx() if hasattr(b,'read_bsx') else b.read_bs()
    ltscale=b.read_bd()
    ltype_flags=b.read_bb()
    plotstyle=b.read_bb()
    invisible=b.read_bs()
    linewt=b.read_rc()
    print(f"  preview={preview} entmode={entmode} num_reactors={numr} xdic_missing={xdic}")
    print(f"  color.raw=0x{raw:X} index={raw&0x1ff} ltype_scale={ltscale} ltype_flags={ltype_flags} plotstyle={plotstyle} invisible={invisible} linewt=0x{linewt:X}")
    # hdl_dat 独立流：从 bitsize 定位
    print(f"  dat @bit={b.tell()} byte={b.byte} bit={b.bit}")
    # LINE 几何
    zzero=b.read_b()
    sx=b.read_rd(); sy=b.read_rd()
    ex=b.read_dd(sx); ey=b.read_dd(sy)
    print(f"  z_is_zero={zzero} start=({sx},{sy}) end=({ex},{ey})  expect={expect}")

decode_line(140531, "(-300,200)-(-300,-200)")
decode_line(140595, "(-300,200)-(300,200)")
