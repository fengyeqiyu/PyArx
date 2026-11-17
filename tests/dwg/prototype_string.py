# -*- coding: utf-8 -*-
"""
prototype_string.py —— AC1021 单对象字符串流定位原型（只读取证，不改解析器）。

目标样本：G:\\PyArx\\tests\\楼层平面-10KV电缆桥架.dwg
  目标 A：LAYER，对象起点字节 32187，真值名 E-CABL-TRAY（位级命中 32206.1）
  目标 B：MTEXT，对象起点字节 10999，真值文本片段 8890.00012,...（位级命中 11668.7）

每一步都打印位位置 / 原始码值；§7 的两套回退公式（ACS 16 字节、LDW byte-2）
各自落点都与实测串起点对照，偏差在报告里显式给出。

位码语义参考：ODA R2007 规范 §2；ACS MIT 实现 DwgStreamReaderBase / DwgObjectReader；
LDW GPLv3 仅读结构（decode_r2007.c obj_string_stream / section_string_stream）。
"""
from __future__ import annotations

import os
import sys
from dataclasses import dataclass, field
from typing import Optional, Tuple

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)

from PyArxDwg.container import open_dwg  # noqa: E402
from PyArxDwg.objects import Reader  # noqa: E402

DWG = os.path.join(REPO, "tests", "楼层平面-10KV电缆桥架.dwg")

# 实测位级命中（由全段位扫描得到，作为对照基准，不参与公式推导）
TRUTH = {
    "A": {"offset": 32187, "type": 0x33, "label": "LAYER",
          "prefix_bit": 32204 * 8 + 7, "char_bit": 32206 * 8 + 1,
          "truth": "E-CABL-TRAY"},
    "B": {"offset": 10999, "type": 0x2C, "label": "MTEXT",
          "prefix_bit": 11668 * 8 + 7, "char_bit": 11671 * 8 + 1,
          "truth": "8890.00012,9207.49987,9525,9842.50012,10159.99988"},
}


# ---------------------------------------------------------------------------
# 头部 / 流定位
# ---------------------------------------------------------------------------

@dataclass
class Trace:
    offset: int
    first16: bytes = b""
    ms: int = 0
    type_code: int = 0
    bitsize: int = 0
    pos_after_ms: int = 0
    pos_after_type: int = 0
    pos_after_rl: int = 0
    handle_bit: int = 0
    flag: Optional[int] = None
    acs: dict = field(default_factory=dict)
    ldw: dict = field(default_factory=dict)
    empirical_tv: str = ""
    notes: list = field(default_factory=list)


def read_ms(r: Reader) -> int:
    """MS 模块化短：RS；若 0x8000 再读 RS，低 15 位 + 高字 <<15（bits.c:1123）。"""
    lo = r.rs()
    if lo & 0x8000:
        hi = r.rs()
        return (lo & 0x7FFF) | (hi << 15)
    return lo


def parse_header(obj: bytes, offset: int) -> Trace:
    t = Trace(offset=offset)
    t.first16 = obj[offset:offset + 16]
    r = Reader(obj, offset * 8)
    t.ms = read_ms(r)
    t.pos_after_ms = r.pos
    t.type_code = r.bs()
    t.pos_after_type = r.pos
    t.bitsize = r.rl()
    t.pos_after_rl = r.pos
    t.handle_bit = offset * 8 + t.bitsize

    # §7.2：has_strings 位 = handleStart - 1
    try:
        t.flag = Reader(obj, t.handle_bit - 1).bit()
    except Exception as exc:  # noqa: BLE001
        t.notes.append(f"flag read error: {type(exc).__name__}: {exc}")

    # ACS 回退：lastPos-16（位）与 -16 字节两套单位都试；RS data_size；0x8000 分支
    hdl_byte = t.handle_bit // 8
    for unit, back in (("bits16", 16), ("bytes16", 128)):
        L = t.handle_bit - back
        try:
            lo = Reader(obj, L).rs()
            size = lo
            hi = 0
            if lo & 0x8000:
                L2 = L - back
                hi = Reader(obj, L2).rs()
                size = (lo & 0x7FFF) | (hi << 15)
            t.acs[unit] = {"L_bit": L, "lo": lo, "hi": hi, "size": size,
                           "str_bit": L - size * 8, "str_byte": (L - size * 8) // 8}
        except Exception as exc:  # noqa: BLE001
            t.acs[unit] = {"error": f"{type(exc).__name__}: {exc}"}

    # LDW 回退：flag 后退 1 位、字节 -2 读 RS；0x8000 字节 -4 读 hi；再字节 -2；退 size 字节
    b2 = hdl_byte - 2
    try:
        lo = Reader(obj, b2 * 8).rs()
        size = lo
        hi = 0
        if lo & 0x8000:
            b3 = b2 - 4
            hi = Reader(obj, b3 * 8).rs()
            size = (lo & 0x7FFF) | (hi << 15)
        b4 = b2 - 2
        t.ldw = {"b2": b2, "lo": lo, "hi": hi, "size": size, "b4": b4,
                 "str_byte": b4 - size, "str_bit": (b4 - size) * 8}
    except Exception as exc:  # noqa: BLE001
        t.ldw = {"error": f"{type(exc).__name__}: {exc}"}

    # 尾部 RS 扫描：找 80~180 字节的可疑 data_size 字段（实测对照用）
    tail = []
    for b in range(max(0, hdl_byte - 80), min(len(obj) - 1, hdl_byte)):
        lo = Reader(obj, b * 8).rs()
        size = lo
        hi = 0
        if lo & 0x8000:
            hi = Reader(obj, (b - 2) * 8).rs()
            size = (lo & 0x7FFF) | (hi << 15)
        if 80 <= size <= 180:
            tail.append({"byte": b, "lo": lo, "size": size, "L-size": b - size})
    t.acs["tail_scan"] = tail

    return t


def read_tv_at(obj: bytes, bit: int) -> Tuple[str, int, int]:
    """在给定位位置按 TV 读：BS 2 位长度码 + 长度 + UTF-16LE；返回 (串, 码, 长度)。"""
    r = Reader(obj, bit)
    code = r.bb()
    if code == 0:
        length = r.rs()
    elif code == 1:
        length = r.rc()
    elif code == 2:
        length = 0
    else:
        length = 256
    if length <= 0:
        return "", code, length
    blob = bytes(r.rc() for _ in range(length * 2))
    return blob.rstrip(b"\x00\x00").decode("utf-16-le", errors="replace"), code, length


def report(name: str, obj: bytes) -> None:
    truth = TRUTH[name]
    print(f"\n===== 目标 {name}：{truth['label']} @ byte {truth['offset']} =====")
    t = parse_header(obj, truth["offset"])

    print(f"前 16 字节: {t.first16.hex()}")
    print(f"MS size(字节) = {t.ms}  读完位 {t.pos_after_ms}")
    print(f"BS type = 0x{t.type_code:X}（期望 0x{truth['type']:X}）  读完位 {t.pos_after_type}")
    print(f"RL bitsize(位) = {t.bitsize}  读完位 {t.pos_after_rl}")
    print(f"handleStartBit = objPos({truth['offset'] * 8}) + bitsize({t.bitsize}) "
          f"= {t.handle_bit}（byte {t.handle_bit // 8}, k {t.handle_bit % 8}）")
    print(f"has_strings @ handleStart-1 = {t.flag}")

    print("\n-- ACS 回退（-16 位 / -16 字节 两套单位）--")
    for unit, d in t.acs.items():
        print(f"  [{unit}] {d}")
    print("-- LDW 回退（byte-2 RS, 0x8000 byte-4 hi, byte-2, 退 size 字节）--")
    for k, v in t.ldw.items():
        print(f"  {k}: {v}")

    # 实测 TV 解码（基准位位置）
    text, code, length = read_tv_at(obj, truth["prefix_bit"])
    t.empirical_tv = text
    print(f"\n实测 TV @ bit {truth['prefix_bit']}（byte {truth['prefix_bit'] // 8}, "
          f"k {truth['prefix_bit'] % 8}）: code={code} len={length} text={text!r}")
    print(f"真值: {truth['truth']!r}  匹配={text.startswith(truth['truth'][:len(text)])}")

    # 公式落点 vs 实测前缀位
    actual = truth["prefix_bit"]
    for scheme, d in (("ACS.bits16", t.acs.get("bits16", {})),
                      ("ACS.bytes16", t.acs.get("bytes16", {})),
                      ("LDW", t.ldw)):
        if "str_bit" in d:
            delta = d["str_bit"] - actual
            print(f"{scheme} 公式串起点 bit {d['str_bit']}，偏差 {delta:+d} 位 "
                  f"（实测 {actual}）")
        else:
            print(f"{scheme} 公式未给出落点: {d.get('error')}")

    # 一行算术：相对对象起点
    rel_prefix = actual - truth["offset"] * 8
    print(f"\n算术: objPos={truth['offset'] * 8} bitsize={t.bitsize} "
          f"string_prefix={actual}  rel(prefix-objPos)={rel_prefix}  "
          f"bitsize-rel={t.bitsize - rel_prefix}")


def main() -> int:
    container = open_dwg(DWG)
    obj = container.section_bytes("AcDb:AcDbObjects")
    print(f"对象段长度 {len(obj)} 字节；DWG {DWG}")
    report("A", obj)
    report("B", obj)
    return 0


if __name__ == "__main__":
    sys.exit(main())
