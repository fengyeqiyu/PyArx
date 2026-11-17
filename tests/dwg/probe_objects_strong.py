# -*- coding: utf-8 -*-
"""
probe_objects_strong.py —— 用强电样本句柄映射独立抽检对象头。

只读调用 PyArxDwg.container；不修改解析器。
判据：
  * MS = ReadModularShort，对象字节大小（不含尾部 CRC）；
  * BS = ReadBitShort，对象类型码；
  * 用容器页表判断对象是否跨页；
  * 与 DXF 真值按 handle 配对做类型命中对照。
"""
from __future__ import annotations

import json
import os
import statistics
import sys
from collections import Counter, defaultdict
from typing import Any, Dict, List, Optional, Tuple

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)
sys.path.insert(0, HERE)

from PyArxDwg.container import open_dwg  # noqa: E402
from probe_handles import parse_handles, BitStream  # noqa: E402

DWG = os.path.join(REPO, "tests", "楼层平面-强电.dwg")
TRUTH = os.path.join(HERE, "truth_楼层平面-强电.json")
OUT = os.path.join(HERE, "probe_objects_strong.json")

ENUM_NAMES: Dict[int, str] = {
    0x01: "TEXT", 0x02: "ATTRIB", 0x03: "ATTDEF", 0x04: "BLOCK", 0x05: "ENDBLK",
    0x06: "SEQEND", 0x07: "INSERT", 0x08: "MINSERT", 0x0A: "VERTEX_2D",
    0x0B: "VERTEX_3D", 0x0C: "VERTEX_MESH", 0x0D: "VERTEX_PFACE",
    0x0E: "VERTEX_PFACE_FACE", 0x0F: "POLYLINE_2D", 0x10: "POLYLINE_3D",
    0x11: "ARC", 0x12: "CIRCLE", 0x13: "LINE",
    0x14: "DIMENSION_ORDINATE", 0x15: "DIMENSION_LINEAR",
    0x16: "DIMENSION_ALIGNED", 0x17: "DIMENSION_ANG_3Pt", 0x18: "DIMENSION_ANG_2Ln",
    0x19: "DIMENSION_RADIUS", 0x1A: "DIMENSION_DIAMETER", 0x1B: "POINT",
    0x1C: "FACE3D", 0x1D: "POLYLINE_PFACE", 0x1E: "POLYLINE_MESH", 0x1F: "SOLID",
    0x20: "TRACE", 0x21: "SHAPE", 0x22: "VIEWPORT", 0x23: "ELLIPSE", 0x24: "SPLINE",
    0x25: "REGION", 0x26: "SOLID3D", 0x27: "BODY", 0x28: "RAY", 0x29: "XLINE",
    0x2A: "DICTIONARY", 0x2B: "OLEFRAME", 0x2C: "MTEXT", 0x2D: "LEADER",
    0x2E: "TOLERANCE", 0x2F: "MLINE", 0x30: "BLOCK_CONTROL", 0x31: "BLOCK_HEADER",
    0x32: "LAYER_CONTROL", 0x33: "LAYER", 0x34: "STYLE_CONTROL", 0x35: "STYLE",
    0x38: "LTYPE_CONTROL", 0x39: "LTYPE", 0x3C: "VIEW_CONTROL", 0x3D: "VIEW",
    0x3E: "UCS_CONTROL", 0x3F: "UCS", 0x40: "VPORT_CONTROL", 0x41: "VPORT",
    0x42: "APPID_CONTROL", 0x43: "APPID", 0x44: "DIMSTYLE_CONTROL", 0x45: "DIMSTYLE",
    0x48: "GROUP", 0x49: "MLINESTYLE", 0x4A: "OLE2FRAME", 0x4B: "DUMMY",
    0x4C: "LONG_TRANSACTION", 0x4D: "LWPOLYLINE", 0x4E: "HATCH", 0x4F: "XRECORD",
    0x50: "ACDBPLACEHOLDER", 0x51: "VBA_PROJECT", 0x52: "LAYOUT",
    0x1F2: "ACAD_PROXY_ENTITY", 0x1F3: "ACAD_PROXY_OBJECT",
}

ENTITY_BROAD: Dict[int, str] = {
    0x13: "LINE", 0x4D: "LWPOLYLINE", 0x07: "INSERT", 0x2C: "MTEXT",
    0x11: "ARC", 0x4E: "HATCH", 0x12: "CIRCLE", 0x23: "ELLIPSE",
    0x24: "SPLINE", 0x0F: "POLYLINE", 0x10: "POLYLINE",
    0x0A: "VERTEX", 0x0B: "VERTEX", 0x0C: "VERTEX", 0x0D: "VERTEX", 0x0E: "VERTEX",
    0x14: "DIMENSION", 0x15: "DIMENSION", 0x16: "DIMENSION", 0x17: "DIMENSION",
    0x18: "DIMENSION", 0x19: "DIMENSION", 0x1A: "DIMENSION",
}

EXPECTED = {
    "LINE": 8425, "LWPOLYLINE": 6440, "INSERT": 4547, "MTEXT": 1443,
    "ARC": 768, "HATCH": 162, "CIRCLE": 137, "ELLIPSE": 5, "SPLINE": 6,
    "POLYLINE": 1, "VERTEX": 7,
}


def read_modular_short(data: bytes, pos: int) -> Tuple[int, int]:
    """ACadSharp ReadModularShort：成对字节，b2 高 bit 为延续标记。"""
    start = pos
    b1 = data[pos]
    b2 = data[pos + 1]
    pos += 2
    flag = not (b2 & 0x80)
    value = b1 | ((b2 & 0x7F) << 8)
    shift = 15
    while not flag:
        b1 = data[pos]
        b2 = data[pos + 1]
        pos += 2
        flag = not (b2 & 0x80)
        value |= b1 << shift
        shift += 8
        value |= (b2 & 0x7F) << shift
        shift += 7
    return value, pos - start


def read_bs_type(data: bytes, pos: int) -> Tuple[int, str]:
    """BS / ReadBitShort：0=LE short，1=byte，2=0，3=256。"""
    bit_pos = pos * 8
    shift = 0
    byte = data[pos]
    code = byte >> 6
    if code == 0:
        if pos + 2 > len(data):
            raise EOFError("short overrun")
        raw = data[pos + 1:pos + 3]
        value = int.from_bytes(raw, "little", signed=False)
        return value, "short"
    if code == 1:
        return byte & 0x7F, "byte"
    if code == 2:
        return 0, "zero"
    return 256, "256"


def load_truth() -> Dict[str, str]:
    if not os.path.exists(TRUTH):
        return {}
    with open(TRUTH, encoding="utf-8") as fh:
        truth = json.load(fh)
    return {e["handle"].upper(): e["type"].upper() for e in truth.get("entities", []) if e.get("handle")}


def page_for_offset(pages: List[Tuple[int, int, int, int]], offset: int) -> Optional[Tuple[int, int, int, int]]:
    for page in pages:
        data_offset, _file_offset, _compressed, uncompressed = page
        end = data_offset + uncompressed
        if data_offset <= offset < end:
            return page
    return None


def main() -> int:
    container = open_dwg(DWG)
    objects = container.section_bytes("AcDb:AcDbObjects")
    handle_entries = parse_handles(container.section_bytes("AcDb:Handles"))
    pages = sorted(container.page_mapping("AcDb:AcDbObjects"), key=lambda p: p[0])
    truth = load_truth()

    boundary_errors: List[Dict[str, Any]] = []
    parse_errors: List[Dict[str, Any]] = []
    type_counter: Counter[str] = Counter()
    type_code_counter: Counter[int] = Counter()
    sizes: List[int] = []
    cross_page = 0
    cross_page_list: List[Dict[str, Any]] = []

    compared = 0
    type_hits = 0
    mismatches: List[Dict[str, Any]] = []
    decoded_by_broad: Counter[str] = Counter()
    decoded_truth_only: List[Dict[str, Any]] = []

    for entry in handle_entries:
        handle = f"{entry.handle:X}"
        offset = entry.loc or -1
        record: Dict[str, Any] = {"handle": handle, "offset": offset}

        if offset < 0 or offset >= len(objects):
            boundary_errors.append({**record, "reason": "offset out of section", "section_len": len(objects)})
            continue

        try:
            size, size_width = read_modular_short(objects, offset)
        except (EOFError, IndexError) as exc:
            parse_errors.append({**record, "reason": f"size overrun: {exc}"})
            continue

        record["size"] = size
        record["size_width_bytes"] = size_width
        record["header_hex"] = objects[offset:offset + 16].hex()

        if size <= 0:
            parse_errors.append({**record, "reason": "non-positive size"})
            continue

        end = offset + size
        if end > len(objects):
            boundary_errors.append({
                **record,
                "reason": "size exceeds section",
                "section_len": len(objects),
                "end_offset": end,
            })
            continue

        sizes.append(size)
        type_pos = offset + size_width
        try:
            type_code, type_form = read_bs_type(objects, type_pos)
        except (EOFError, IndexError) as exc:
            parse_errors.append({**record, "reason": f"type overrun: {exc}"})
            continue

        record["type_code"] = type_code
        record["type_name"] = ENUM_NAMES.get(type_code, f"UNKNOWN_{type_code}")
        record["type_form"] = type_form
        broad = ENTITY_BROAD.get(type_code)
        record["broad_type"] = broad
        type_counter[record["type_name"]] += 1
        type_code_counter[type_code] += 1
        if broad:
            decoded_by_broad[broad] += 1

        page = page_for_offset(pages, offset)
        if page is not None:
            page_start, _file, _comp, uncomp = page
            page_end = page_start + uncomp
            if end > page_end:
                cross_page += 1
                if len(cross_page_list) < 20:
                    cross_page_list.append({**record, "page_start": page_start, "page_end": page_end})

        truth_type = truth.get(handle)
        if truth_type:
            decoded_truth_only.append(record)
            compared += 1
            if broad == truth_type:
                type_hits += 1
            else:
                mismatches.append({
                    **record,
                    "truth_type": truth_type,
                })

    mismatch_by_truth: Counter[str] = Counter(m["truth_type"] for m in mismatches)
    mismatch_by_decoded: Counter[str] = Counter(m["broad_type"] or "None" for m in mismatches)

    result: Dict[str, Any] = {
        "source": DWG,
        "section_len": len(objects),
        "handle_entries": len(handle_entries),
        "truth_entity_count": len(truth),
        "pages": [{"start": s, "file_offset": f, "compressed": c, "uncompressed": u}
                  for s, f, c, u in pages],
        "boundary": {
            "error_count": len(boundary_errors),
            "first20": boundary_errors[:20],
        },
        "parse_errors": {
            "count": len(parse_errors),
            "first20": parse_errors[:20],
        },
        "size_stats": {
            "count": len(sizes),
            "min": min(sizes) if sizes else None,
            "max": max(sizes) if sizes else None,
            "median": statistics.median(sizes) if sizes else None,
            "mean": statistics.mean(sizes) if sizes else None,
        },
        "cross_page": {
            "count": cross_page,
            "first20": cross_page_list,
        },
        "type_histogram": dict(sorted(type_counter.items(), key=lambda kv: (-kv[1], kv[0]))),
        "type_code_histogram": {f"0x{k:X}": v for k, v in sorted(type_code_counter.items())},
        "broad_decoded": dict(decoded_by_broad),
        "expected_truth_counts": EXPECTED,
        "truth_compare": {
            "compared": compared,
            "type_hits": type_hits,
            "hit_ratio": (type_hits / compared) if compared else None,
            "mismatch_count": len(mismatches),
            "mismatch_by_truth": dict(mismatch_by_truth),
            "mismatch_by_decoded": dict(mismatch_by_decoded),
            "first30_mismatches": mismatches[:30],
        },
    }

    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(result, fh, ensure_ascii=False, indent=1)

    print(f"section_len={len(objects)} handle_entries={len(handle_entries)} truth={len(truth)}")
    print(f"boundary_errors={len(boundary_errors)} parse_errors={len(parse_errors)}")
    print(f"size: n={len(sizes)} min={result['size_stats']['min']} median={result['size_stats']['median']} "
          f"max={result['size_stats']['max']}")
    print(f"cross_page={cross_page}")
    print(f"truth compared={compared} hits={type_hits} ratio={result['truth_compare']['hit_ratio']}")
    print("broad decoded:", dict(decoded_by_broad))
    print("top types:", result["type_histogram"].most_common(15))
    return 0


if __name__ == "__main__":
    sys.exit(main())
