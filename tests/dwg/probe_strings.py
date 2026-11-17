# -*- coding: utf-8 -*-
"""
probe_strings.py —— AC1021 表项/块名/文字原始字节取证。

只读调用 PyArxDwg.container；不修改解析器。
取证对象：10KV 样本（4 图层、8 块、4 LTYPE、1 个 MTEXT）。
"""
from __future__ import annotations

import json
import os
import sys
from dataclasses import dataclass
from typing import Any, Dict, List, Optional

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)
sys.path.insert(0, HERE)

from PyArxDwg.container import open_dwg  # noqa: E402
from probe_handles import parse_handles  # noqa: E402

DWG = os.path.join(REPO, "tests", "楼层平面-10KV电缆桥架.dwg")
DXF = os.path.join(HERE, "楼层平面-10KV电缆桥架.dxf")
OUT = os.path.join(HERE, "probe_strings_10kv.json")

WINDOW_BYTES = 31


def read_dxf_pairs(path: str) -> List[tuple[str, str]]:
    raw = open(path, "rb").read()
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        text = raw.decode("gbk", errors="replace")
    lines = text.splitlines()
    pairs: List[tuple[str, str]] = []
    i = 0
    while i + 1 < len(lines):
        pairs.append((lines[i].strip(), lines[i + 1].strip()))
        i += 2
    return pairs


def dxf_records(dxf_type: str) -> List[Dict[str, str]]:
    pairs = read_dxf_pairs(DXF)
    records: List[Dict[str, str]] = []
    current: Optional[Dict[str, str]] = None
    for code, value in pairs:
        if code == "0":
            if current is not None:
                records.append(current)
            current = {"type": value} if value == dxf_type else None
        elif current is not None:
            current[code] = value
    if current is not None:
        records.append(current)
    return records


def encodings(name: str) -> Dict[str, bytes]:
    out: Dict[str, bytes] = {}
    for enc in ("utf-16-le", "ascii", "gbk"):
        try:
            out[enc] = name.encode(enc)
        except (UnicodeEncodeError, LookupError):
            pass
    return out


def find_all(haystack: bytes, needle: bytes, limit: int = 5) -> List[int]:
    hits: List[int] = []
    pos = haystack.find(needle)
    while pos >= 0 and len(hits) < limit:
        hits.append(pos)
        pos = haystack.find(needle, pos + 1)
    return hits


def utf16_runs(data: bytes, min_len: int = 3) -> List[Dict[str, Any]]:
    runs: List[Dict[str, Any]] = []
    i = 0
    while i + 1 < len(data):
        lo, hi = data[i], data[i + 1]
        printable = (hi == 0 and 32 <= lo < 127) or (hi != 0 and lo != 0)
        if not printable:
            i += 1
            continue
        j = i
        buf = bytearray()
        while j + 1 < len(data):
            lo2, hi2 = data[j], data[j + 1]
            ok = (hi2 == 0 and 32 <= lo2 < 127) or (hi2 != 0 and lo2 != 0)
            if not ok:
                break
            buf += data[j:j + 2]
            j += 2
        if len(buf) // 2 >= min_len:
            try:
                text = buf.decode("utf-16-le", errors="replace")
                runs.append({"offset": i, "text": text, "hex": buf.hex()})
            except UnicodeDecodeError:
                pass
        i = max(j, i + 1)
    return runs


def main() -> int:
    container = open_dwg(DWG)
    objects = container.section_bytes("AcDb:AcDbObjects")
    handles = parse_handles(container.section_bytes("AcDb:Handles"))
    handle_map = {f"{e.handle:X}": e.loc for e in handles}

    layers = dxf_records("LAYER")
    blocks = dxf_records("BLOCK_RECORD")
    ltypes = dxf_records("LTYPE")
    mtexts = dxf_records("MTEXT")

    def record_row(rec: Dict[str, str], kind: str, name_code: str) -> Dict[str, Any]:
        handle = rec.get("5", "").upper()
        name = rec.get(name_code, "")
        loc = handle_map.get(handle)
        window = objects[loc:loc + WINDOW_BYTES] if loc is not None else b""
        enc = encodings(name)
        whole_hits: Dict[str, List[int]] = {}
        window_hits: Dict[str, List[int]] = {}
        for enc_name, needle in enc.items():
            whole_hits[enc_name] = find_all(objects, needle)
            window_hits[enc_name] = find_all(window, needle)
        return {
            "kind": kind,
            "handle": handle,
            "truth_name": name,
            "truth_ltype": rec.get("6"),
            "object_offset": loc,
            "window_hex": window.hex(),
            "window_hits": window_hits,
            "object_section_hits": whole_hits,
            "found": any(h for h in whole_hits.values()),
        }

    layer_rows = [record_row(r, "LAYER", "2") for r in layers]
    block_rows = [record_row(r, "BLOCK_RECORD", "2") for r in blocks]
    ltype_rows = [record_row(r, "LTYPE", "2") for r in ltypes]
    mtext_row = record_row(mtexts[0], "MTEXT", "3") if mtexts else None

    raw_dwg = open(DWG, "rb").read()
    section_search: Dict[str, Any] = {}
    for row in layer_rows + block_rows + ltype_rows:
        name = row["truth_name"]
        enc = encodings(name)
        per_section: Dict[str, Dict[str, List[int]]] = {}
        for sec_name in container.sections:
            sec_data = container.section_bytes(sec_name)
            per_section[sec_name] = {
                enc_name: find_all(sec_data, needle)
                for enc_name, needle in enc.items()
            }
        section_search[name] = {
            "sections": per_section,
            "raw_dwg": {enc_name: find_all(raw_dwg, needle) for enc_name, needle in enc.items()},
        }

    runs = utf16_runs(objects)
    block_name_set = {r["truth_name"] for r in block_rows}
    matched_runs = [r for r in runs if r["text"] in block_name_set]

    result: Dict[str, Any] = {
        "dwg": DWG,
        "dxf": DXF,
        "object_section_len": len(objects),
        "handle_count": len(handle_map),
        "layers": layer_rows,
        "blocks": block_rows,
        "ltypes": ltype_rows,
        "mtext": mtext_row,
        "section_search": section_search,
        "utf16_runs_matched_blocks": matched_runs,
        "summary": {
            "layer_count": len(layer_rows),
            "layer_found": sum(r["found"] for r in layer_rows),
            "block_count": len(block_rows),
            "block_found": sum(r["found"] for r in block_rows),
            "ltype_count": len(ltype_rows),
            "ltype_found": sum(r["found"] for r in ltype_rows),
        },
    }

    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(result, fh, ensure_ascii=False, indent=1)

    for title, rows in (("LAYER", layer_rows), ("BLOCK", block_rows), ("LTYPE", ltype_rows)):
        print(f"== {title} ==")
        for r in rows:
            hits = {k: v for k, v in r["object_section_hits"].items() if v}
            print(f"  {r['handle']:<6} off={r['object_offset']} found={r['found']} "
                  f"name={r['truth_name']!r} hits={hits}")
            print(f"    window={r['window_hex']}")
    if mtext_row:
        print(f"== MTEXT {mtext_row['handle']} off={mtext_row['object_offset']} ==")
        print(f"  name={mtext_row['truth_name']!r} found={mtext_row['found']}")
        print(f"  window={mtext_row['window_hex']}")
    print("summary:", result["summary"])
    print("matched UTF16 block runs:", matched_runs)
    return 0


if __name__ == "__main__":
    sys.exit(main())
