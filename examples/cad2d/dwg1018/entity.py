# -*- coding: utf-8 -*-
"""通用 DWG 实体离线解码器（AC1018 / R2004）。

公共头部 + 公共 dat 字段与类型几何段分离；类型几何由 acdb.py 实体类体系
按 type code 自动分发读取（仿 SDK：基类 AcDbEntity + 类型子类）。
缓冲：obj_full.bin（Objects 节解压流）。句柄流 hdl_dat 不在本模块（#13）。
"""
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from bitio import BitStream
import acdb

DATA = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin", "rb").read()

# dwg.h 类型枚举 -> 内部名
TYPE_NAME = {
    0x01: "TEXT", 0x02: "ATTRIB", 0x03: "ATTDEF", 0x07: "INSERT", 0x11: "ARC",
    0x12: "CIRCLE", 0x13: "LINE", 0x1b: "POINT", 0x23: "ELLIPSE", 0x24: "SPLINE",
    0x2c: "MTEXT", 0x4d: "LWPOLYLINE", 0x4e: "HATCH",
    0x14: "DIMENSION", 0x15: "DIMENSION", 0x16: "DIMENSION",
    0x17: "DIMENSION", 0x18: "DIMENSION", 0x19: "DIMENSION",
    0x1a: "DIMENSION",
    0x330: "TABLE",
}

# 兼容旧引用：类型名 -> 实体类
GEOM = acdb.BY_NAME


def read_h(b):
    c = b.read_rc(); sz = c & 0xf; v = 0
    for _ in range(sz):
        v = (v << 8) | b.read_rc()
    return c, sz, v


def decode_eed(b):
    end = None; q = None
    while True:
        size = b.read_bs()
        if size == 0 or size > 1000:
            break
        read_h(b)
        end = b.byte + size
        q = BitStream(DATA); q.byte = b.byte; q.bit = b.bit
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


def read_header(b):
    """对象头：type BS, bitsize RL, handle H。返回 (type, bitsize, handle)。"""
    type_code = b.read_bs()
    bitsize = b.read_rl()
    _, _, handle = read_h(b)
    return type_code, bitsize, handle


def read_common(b):
    """公共 dat 段（EED 之后、类型几何之前）。返回 dict。"""
    preview = b.read_b()
    if preview:
        pvsz = b.read_rl()              # preview_size RL
        b.read_tf(pvsz)                 # preview 二进制（跳过）
    entmode = b.read_bb()
    num_reactors = b.read_bl()
    xdic_missing = b.read_b()
    raw = b.read_bs()                  # color raw (ENC)
    flag = raw >> 8
    if flag & 0x80: b.read_bl()      # rgb BL
    if flag & 0x20: b.read_bl()      # alpha_raw BL
    if (flag & 0x41) == 0x41: b.read_tv()
    if (flag & 0x42) == 0x42: b.read_tv()
    b.read_bd()                      # ltscale
    ltype_flags = b.read_bb()
    plotstyle_flags = b.read_bb()
    b.read_bs()                      # invisible
    b.read_rc()                      # linewt
    return {"color": raw & 0x1ff, "raw": raw, "entmode": entmode,
            "num_reactors": num_reactors, "xdic_missing": xdic_missing,
            "ltype_flags": ltype_flags, "plotstyle_flags": plotstyle_flags}


def open_handles(addr, bitsize):
    h = BitStream(DATA)
    h.byte = addr + bitsize // 8
    h.bit = bitsize % 8
    return h


def read_handles(h, common):
    """对象句柄流：起点 = 主流起点 + bitsize 位。返回 handle dict，h 流状态保留给 geom 引用句柄。"""
    out = {"owner": 0, "reactors": [], "xdic": 0, "layer": 0, "ltype": 0, "plotstyle": 0}
    if common["entmode"] == 0:
        _, _, out["owner"] = read_h(h)
    for _ in range(common["num_reactors"]):
        _, _, r = read_h(h)
        out["reactors"].append(r)
    if not common["xdic_missing"]:
        _, _, out["xdic"] = read_h(h)
    _, _, out["layer"] = read_h(h)
    if common["ltype_flags"] == 3:
        _, _, out["ltype"] = read_h(h)
    if common["plotstyle_flags"] == 3:
        _, _, out["plotstyle"] = read_h(h)
    return out


def decode_object(addr):
    """解码单个对象，返回实体类实例（仿 SDK openObject + cast）；未知类型返回 None。"""
    b = BitStream(DATA); b.byte = addr
    type_code, bitsize, handle = read_header(b)
    decode_eed(b)
    common = read_common(b)
    h = open_handles(addr, bitsize)
    common["handles"] = read_handles(h, common)
    cls = acdb.BY_CODE.get(type_code)
    if cls is None:
        return None
    obj = cls(handle, common)
    obj.dxfName = TYPE_NAME.get(type_code, cls.dxfName)
    obj.geom = obj.readGeom(b, h)
    return obj


def decode_entity(addr):
    """解析单个对象。返回 (type_name, handle, common, geom)。"""
    obj = decode_object(addr)
    if obj is None:
        b = BitStream(DATA); b.byte = addr
        type_code, _, handle = read_header(b)
        name = TYPE_NAME.get(type_code, "UNK_%d" % type_code)
        return name, handle, {}, None
    return obj.dxfName, obj.handle, obj.common, obj.geom


def decode_table(addr):
    """表记录（LAYER/LTYPE/STYLE/DIMSTYLE 等）：头/EED/numr/xm/name TV + hdl 流。"""
    b = BitStream(DATA); b.byte = addr
    type_code, bitsize, handle = read_header(b)
    decode_eed(b)
    num_reactors = b.read_bl()
    xdic_missing = b.read_b()
    name = b.read_tv()
    hd = BitStream(DATA)
    hd.byte = addr + bitsize // 8
    hd.bit = bitsize % 8
    _, _, owner = read_h(hd)
    reactors = [read_h(hd)[2] for _ in range(num_reactors)]
    xdic = 0
    if not xdic_missing:
        _, _, xdic = read_h(hd)
    return type_code, handle, name, {"owner": owner, "reactors": reactors,
                                     "xdic": xdic, "num_reactors": num_reactors,
                                     "xdic_missing": xdic_missing}


def decode_dict(addr):
    """DICTIONARY(42) / DICTIONARYWDFLT(500)：主流 numitems/cloning/hardowner/texts；
    hdl 流 owner/reactors/xdic 后接 itemhandles H[numitems]。"""
    b = BitStream(DATA); b.byte = addr
    type_code, bitsize, handle = read_header(b)
    decode_eed(b)
    num_reactors = b.read_bl()
    xdic_missing = b.read_b()
    numitems = b.read_bl()
    cloning = b.read_bs()
    hardowner = b.read_rc()
    texts = [b.read_tv() for _ in range(numitems)]
    hd = open_handles(addr, bitsize)
    _, _, owner = read_h(hd)
    reactors = [read_h(hd)[2] for _ in range(num_reactors)]
    xdic = 0
    if not xdic_missing:
        _, _, xdic = read_h(hd)
    itemhandles = [read_h(hd)[2] for _ in range(numitems)]
    items = [(texts[i], itemhandles[i]) for i in range(numitems)]
    return type_code, handle, {"owner": owner, "reactors": reactors, "xdic": xdic,
                               "cloning": cloning, "hardowner": hardowner, "items": items}


def decode_dictvar(addr):
    """DICTIONARYVAR(503)：主流 schema RC + strvalue T；hdl 流 owner/reactors/xdic。"""
    b = BitStream(DATA); b.byte = addr
    type_code, bitsize, handle = read_header(b)
    decode_eed(b)
    num_reactors = b.read_bl()
    xdic_missing = b.read_b()
    schema = b.read_rc()
    strvalue = b.read_tv()
    hd = open_handles(addr, bitsize)
    _, _, owner = read_h(hd)
    reactors = [read_h(hd)[2] for _ in range(num_reactors)]
    xdic = 0
    if not xdic_missing:
        _, _, xdic = read_h(hd)
    return type_code, handle, {"owner": owner, "reactors": reactors, "xdic": xdic,
                               "schema": schema, "strvalue": strvalue}
