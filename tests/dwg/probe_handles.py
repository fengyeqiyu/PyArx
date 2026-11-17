# -*- coding: utf-8 -*-
"""
probe_handles.py —— AC1021 AcDb:Handles / AcDb:Classes 段原型探明。

只读调用 ``PyArxDwg.container.open_dwg``；不修改解析器。

输出：
  * Handles 段条目数、前 20 / 末 20 条候选 handle→loc 映射；
  * 与 DXF 真值组码 5 的 handle 集合命中率、差集；
  * Classes 段类号 / app 名 / C++ 类名候选，并对照 DXF 实体类型；
  * handles_10kv.json / handles_strong.json 映射结果。

格式依据（ODA Open Design Specification v5.1 §2.13、§8、§20；ACadSharp MIT 实现）：
  * handle 引用：CODE(4) | COUNTER(4) | 大端 handle 值；
  * BS = 位短：0=short，1=byte，2=0；
  * TU = BS 长度 + UTF-16LE 字符；
  * Classes 段：固定 SN 16B，其后 RL/RL/BL/B 头与重复 classnum/flags/app/name 记录。
"""
from __future__ import annotations

import json
import os
import struct
import sys
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Tuple

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)

from PyArxDwg.container import open_dwg  # noqa: E402

DWG_10KV = os.path.join(REPO, "tests", "楼层平面-10KV电缆桥架.dwg")
DWG_STRONG = os.path.join(REPO, "tests", "楼层平面-强电.dwg")
DXF_10KV = os.path.join(HERE, "楼层平面-10KV电缆桥架.dxf")
DXF_STRONG = os.path.join(HERE, "楼层平面-强电.dxf")


# ---------------------------------------------------------------------------
# 位/字节读取工具
# ---------------------------------------------------------------------------

class BitStream:
    """R2007 对象/句柄位流（MSB first），BS/TU 按 ODA §2.13/§8 解码。"""

    def __init__(self, data: bytes, bit_pos: int = 0):
        self.data = data
        self.byte_pos = bit_pos >> 3
        self.shift = bit_pos & 7
        self.last = 0
        if self.shift:
            self._advance()

    def remaining_bits(self) -> int:
        return len(self.data) * 8 - self.position_bits()

    def position_bits(self) -> int:
        return self.byte_pos * 8 + self.shift - 8

    def _advance(self) -> None:
        if self.byte_pos >= len(self.data):
            raise EOFError("bit stream overrun")
        self.last = self.data[self.byte_pos]
        self.byte_pos += 1

    def read_bit(self) -> int:
        if self.shift == 0:
            self._advance()
            return int(bool(self.last & 0x80))
        v = bool((self.last << self.shift) & 0x80)
        self.shift = (self.shift + 1) & 7
        return int(v)

    def read_2bits(self) -> int:
        if self.shift == 0:
            self._advance()
            v = self.last >> 6
            self.shift = 2
            return v
        if self.shift == 7:
            tail = (self.last << 1) & 2
            self._advance()
            v = tail | (self.last >> 7)
            self.shift = 1
            return v
        v = (self.last >> (6 - self.shift)) & 3
        self.shift = (self.shift + 2) & 7
        return v

    def read_byte(self) -> int:
        if self.shift == 0:
            self._advance()
            return self.last
        tail = (self.last << self.shift) & 0xFF
        self._advance()
        return (tail | (self.last >> (8 - self.shift))) & 0xFF

    def read_bytes(self, n: int) -> bytes:
        out = bytearray(n)
        if self.shift == 0:
            for i in range(n):
                self._advance()
                out[i] = self.last
            return bytes(out)
        for i in range(n):
            tail = (self.last << self.shift) & 0xFF
            self._advance()
            out[i] = (tail | (self.last >> (8 - self.shift))) & 0xFF
        return bytes(out)

    def read_short(self) -> int:
        return struct.unpack_from("<h", self.read_bytes(2), 0)[0]

    def read_ushort(self) -> int:
        return struct.unpack_from("<H", self.read_bytes(2), 0)[0]

    def read_long(self) -> int:
        return struct.unpack_from("<i", self.read_bytes(4), 0)[0]

    def bs(self) -> int:
        """位短：0=short，1=byte，2=0，3=256。"""
        code = self.read_2bits()
        if code == 0:
            return self.read_short()
        if code == 1:
            return self.read_byte()
        if code == 2:
            return 0
        return 256

    def bl(self) -> int:
        """位长：0=long，1=byte，2=0。"""
        code = self.read_2bits()
        if code == 0:
            return self.read_long()
        if code == 1:
            return self.read_byte()
        return 0

    def tu(self) -> str:
        """TU：BS 长度 + UTF-16LE 字符串（剔除 NUL）。"""
        length = self.bs()
        if length <= 0:
            return ""
        raw = self.read_bytes(length * 2)
        return raw.decode("utf-16-le", errors="replace").rstrip("\0")

    def handle_reference(self) -> Tuple[int, int, int]:
        """返回 (code, counter, handle 值)；值按小端组装（R2007 对象内句柄惯例）。"""
        form = self.read_byte()
        code = form >> 4
        counter = form & 0x0F
        if counter > 8:
            raise ValueError(f"invalid handle counter {counter}")
        raw = self.read_bytes(counter)
        return code, counter, int.from_bytes(raw, "little")


# ---------------------------------------------------------------------------
# DXF 真值读取（直接 grep 组码 5；复用现有真值 JSON 优先）
# ---------------------------------------------------------------------------

def dxf_handles(dxf_path: str) -> List[str]:
    handles: List[str] = []
    if not os.path.exists(dxf_path):
        return handles
    with open(dxf_path, "rb") as fh:
        raw = fh.read()
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        text = raw.decode("gbk", errors="replace")
    lines = text.splitlines()
    i = 0
    while i + 1 < len(lines):
        code = lines[i].strip()
        if code == "5":
            handles.append(lines[i + 1].strip().upper())
        i += 2
    return handles


def truth_handles_json(dxf_path: str) -> List[str]:
    base = os.path.splitext(os.path.basename(dxf_path))[0]
    cache = os.path.join(HERE, f"truth_{base}.json")
    if not os.path.exists(cache):
        return dxf_handles(dxf_path)
    j = json.load(open(cache, encoding="utf-8"))
    return sorted({(e.get("handle") or "").upper() for e in j.get("entities", []) if e.get("handle")})


# ---------------------------------------------------------------------------
# Handles 段探明
# ---------------------------------------------------------------------------

@dataclass
class HandleEntry:
    raw: bytes
    code: int
    counter: int
    handle: int
    loc: Optional[int] = None
    loc_bytes: Optional[bytes] = None


def _unsigned_modular(data: bytes, pos: int) -> Tuple[int, int]:
    """ACadSharp ReadModularChar：低 7bit，高 bit 为后续字节标记。"""
    start = pos
    b = data[pos]
    value = b & 0x7F
    pos += 1
    shift = 7
    while b & 0x80:
        b = data[pos]
        value |= (b & 0x7F) << shift
        pos += 1
        shift += 7
    return value, pos - start


def _signed_modular(data: bytes, pos: int) -> Tuple[int, int]:
    """ACadSharp ReadSignedModularChar：低 6bit 值，0x40 为符号位。"""
    start = pos
    b = data[pos]
    pos += 1
    if not (b & 0x80):
        value = b & 0x3F
        if b & 0x40:
            value = -value
        return value, pos - start
    total_shift = 0
    total = b & 0x7F
    while True:
        b = data[pos]
        pos += 1
        if b & 0x80:
            total |= (b & 0x7F) << total_shift
        else:
            break
        total_shift += 7
    value = total | ((b & 0x3F) << total_shift)
    if b & 0x40:
        value = -value
    return value, pos - start


def parse_handles(section: bytes) -> List[HandleEntry]:
    """
    R2007 对象映射（ODA §20 / ACadSharp DwgHandleReader）：
      * 每个对象映射小节：BE short 小节长度；size==2 表示结束；
      * 小节内重复：handle 增量 unsigned modular char，位置增量 signed modular char；
      * 每节后 2 字节 CRC。
    """
    entries: List[HandleEntry] = []
    pos = 0
    last_handle = 0
    last_loc = 0
    while pos + 2 <= len(section):
        size = struct.unpack_from(">H", section, pos)[0]
        pos += 2
        if size == 2:
            break
        last_handle = 0
        last_loc = 0
        end = pos + min(size - 2, 2032)
        while pos < end:
            dh, nh = _unsigned_modular(section, pos)
            pos += nh
            dl, nl = _signed_modular(section, pos)
            pos += nl
            last_handle += dh
            last_loc += dl
            raw = section[pos - nh - nl:pos]
            entries.append(HandleEntry(raw, 0, nh, last_handle, last_loc,
                                      raw[nh:nh + nl]))
        pos += 2  # section CRC
    return entries


def handle_set(entries: List[HandleEntry]) -> set[str]:
    return {f"{e.handle:X}" for e in entries}


def _entry_json(e: HandleEntry) -> Dict[str, Any]:
    return {
        "code": e.code,
        "counter": e.counter,
        "handle": f"{e.handle:X}",
        "loc": e.loc,
        "raw_hex": e.raw.hex(),
        "loc_hex": (e.loc_bytes or b"").hex(),
    }


def analyze_handles(dwg: str, dxf: str) -> Dict[str, Any]:
    c = open_dwg(dwg)
    section = c.section_bytes("AcDb:Handles")
    entries = parse_handles(section)
    dwg_set = handle_set(entries)
    truth = truth_handles_json(dxf)
    truth_set = set(truth)
    hit = sorted(dwg_set & truth_set)
    missing = sorted(truth_set - dwg_set)
    extra = sorted(dwg_set - truth_set)

    locs = [e.loc for e in entries if e.loc is not None]
    return {
        "dwg": dwg,
        "section_len": len(section),
        "entry_count": len(entries),
        "first20": [_entry_json(entry) for entry in entries[:20]],
        "last20": [_entry_json(entry) for entry in entries[-20:]],
        "truth_count": len(truth_set),
        "hit_count": len(hit),
        "hit_ratio": (len(hit) / len(truth_set)) if truth_set else None,
        "missing_in_dwg": missing,
        "extra_in_dwg": extra,
        "loc_min": min(locs) if locs else None,
        "loc_max": max(locs) if locs else None,
    }


# ---------------------------------------------------------------------------
# Classes 段探明
# ---------------------------------------------------------------------------

def parse_classes(section: bytes) -> Dict[str, Any]:
    """
    ODA §8：SN(16) + RL 类数据区 + RL 位尺寸 + BL 最大类号 + B；
    重复 classnum(BS) / proxy(BS) / app(TU) / C++ 类名(TU)。
    """
    SN = bytes.fromhex("8da1c4b8c4a9f8c5c0dcf45fe7cfb68a")
    out: Dict[str, Any] = {
        "sn_match": section[:16] == SN,
        "header": {},
        "classes": [],
    }
    if len(section) < 16:
        out["error"] = "section too short"
        return out

    bs = BitStream(section, 16 * 8)
    try:
        data_area = bs.read_long()
        flag_bit_delta = bs.read_long()
        bitlong = bs.bl()
        has_stream = bs.read_bit()
        out["header"] = {
            "class_data_area": data_area,
            "flag_bit_delta": flag_bit_delta,
            "preamble_bitlong": bitlong,
            "string_stream_present": bool(has_stream),
        }
        record = 0
        while bs.remaining_bits() >= 8 and record < 1024:
            classnum = bs.bs()
            proxy = bs.bs()
            app = bs.tu()
            cpp = bs.tu()
            dxf_name = bs.tu()
            zombie = bs.read_bit()
            item_class_id = bs.bs()
            instance_count = bs.bl()
            dwg_version = bs.bl()
            maintenance = bs.bl()
            unknown_a = bs.bl()
            unknown_b = bs.bl()
            out["classes"].append({
                "index": record,
                "classnum": classnum,
                "proxy_flags": proxy,
                "app_name": app,
                "cpp_class_name": cpp,
                "dxf_name": dxf_name,
                "was_zombie": bool(zombie),
                "item_class_id": item_class_id,
                "instance_count": instance_count,
                "dwg_version": dwg_version,
                "maintenance_version": maintenance,
                "unknown_a": unknown_a,
                "unknown_b": unknown_b,
            })
            record += 1
    except (EOFError, ValueError, struct.error, IndexError) as exc:
        out["parse_error"] = f"{type(exc).__name__}: {exc}"
    return out


def dxf_class_records(dxf_path: str) -> List[Dict[str, Any]]:
    """从 DXF CLASS 段抽取类号→DXF 类型（真值侧对照，不依赖 DWG Classes 解码）。"""
    rows: List[Dict[str, Any]] = []
    if not os.path.exists(dxf_path):
        return rows
    raw = open(dxf_path, "rb").read()
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        text = raw.decode("gbk", errors="replace")
    lines = text.splitlines()
    cur: Optional[Dict[str, str]] = None
    i = 0
    while i + 1 < len(lines):
        code = lines[i].strip()
        value = lines[i + 1].strip()
        if code == "0":
            if cur and cur.get("type") == "CLASS":
                rows.append({
                    "classnum": int(cur.get("90", "-1")),
                    "dxf_name": cur.get("1", ""),
                    "cpp_class_name": cur.get("2", ""),
                    "app_name": cur.get("3", ""),
                    "item_class_id": int(cur.get("91", "-1")),
                    "was_zombie": cur.get("280", "0") == "1",
                    "proxy_flags": int(cur.get("281", "-1")),
                })
            cur = {"type": value} if value == "CLASS" else None
        elif cur is not None:
            cur[code] = value
        i += 2
    if cur and cur.get("type") == "CLASS":
        rows.append({
            "classnum": int(cur.get("90", "-1")),
            "dxf_name": cur.get("1", ""),
            "cpp_class_name": cur.get("2", ""),
            "app_name": cur.get("3", ""),
            "item_class_id": int(cur.get("91", "-1")),
            "was_zombie": cur.get("280", "0") == "1",
            "proxy_flags": int(cur.get("281", "-1")),
        })
    return rows


def entity_types_from_dxf(dxf_path: str) -> List[str]:
    types: List[str] = []
    if not os.path.exists(dxf_path):
        return types
    with open(dxf_path, "rb") as fh:
        raw = fh.read()
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        text = raw.decode("gbk", errors="replace")
    lines = text.splitlines()
    i = 0
    while i + 1 < len(lines):
        code = lines[i].strip()
        value = lines[i + 1].strip()
        if code == "0":
            types.append(value.upper())
        i += 2
    seen: List[str] = []
    for t in types:
        if t and t not in seen:
            seen.append(t)
    return seen


def analyze_classes(dwg: str, dxf: str) -> Dict[str, Any]:
    c = open_dwg(dwg)
    cls = parse_classes(c.section_bytes("AcDb:Classes"))
    dxf_types = entity_types_from_dxf(dxf)
    class_names = [x.get("dxf_name", "") or x.get("cpp_class_name", "") for x in cls["classes"]]
    matched = [t for t in dxf_types if any(t in n or n.endswith(t) for n in class_names if n)]
    cls["dxf_entity_types"] = dxf_types
    cls["matched_type_names"] = matched
    cls["dxf_class_table"] = dxf_class_records(dxf)
    return cls


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    analyses = {
        "10kv": analyze_handles(DWG_10KV, DXF_10KV),
        "strong": analyze_handles(DWG_STRONG, DXF_STRONG),
    }
    classes = {
        "10kv": analyze_classes(DWG_10KV, DXF_10KV),
        "strong": analyze_classes(DWG_STRONG, DXF_STRONG),
    }

    for key, item in analyses.items():
        print(f"== Handles {key} ==")
        print(f"section_len={item['section_len']} entries={item['entry_count']} "
              f"truth={item['truth_count']} hit={item['hit_count']} "
              f"ratio={item['hit_ratio']}")
        print("first20:", json.dumps(item["first20"], ensure_ascii=False)[:600])
        print("last20 :", json.dumps(item["last20"], ensure_ascii=False)[:600])
        if item["missing_in_dwg"]:
            print("missing:", item["missing_in_dwg"][:30])
        if item["extra_in_dwg"]:
            print("extra:", item["extra_in_dwg"][:30])

    for key, item in classes.items():
        print(f"== Classes {key} ==")
        print("header:", json.dumps(item.get("header", {}), ensure_ascii=False))
        print("classes:")
        for row in item["classes"][:50]:
            print(f"  #{row['index']:>3} num={row['classnum']:<6} flags={row['proxy_flags']:<5} "
                  f"app={row['app_name']!r} dxf={row['dxf_name']!r} cpp={row['cpp_class_name']!r}")
        print("DXF types:", ", ".join(item["dxf_entity_types"][:30]))
        print("matched :", ", ".join(item["matched_type_names"][:30]))
        print("DXF class table:")
        for row in item["dxf_class_table"]:
            print(f"  #{row['classnum']:<4} {row['dxf_name']:<28} {row['cpp_class_name']:<32} item={row['item_class_id']}")

    with open(os.path.join(HERE, "handles_10kv.json"), "w", encoding="utf-8", newline="\n") as fh:
        json.dump(analyses["10kv"], fh, ensure_ascii=False, indent=1)
    with open(os.path.join(HERE, "handles_strong.json"), "w", encoding="utf-8", newline="\n") as fh:
        json.dump(analyses["strong"], fh, ensure_ascii=False, indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())
