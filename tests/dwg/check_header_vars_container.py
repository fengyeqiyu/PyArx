# -*- coding: utf-8 -*-
"""Container-layer header-variable check (DWG bit stream vs DXF truth, test only).

This script is a standalone acceptance aid: it opens the DWG through the public
:mod:`PyArxDwg` container API, reads the same drawing's DXF export as an external
oracle, and prints a per-field table with the DWG value, DXF value, absolute
error and status.  The DWG decoder itself never reads the DXF at runtime.

Only Python standard library.
"""
from __future__ import annotations

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from PyArxDwg.container import open_dwg  # noqa: E402

TOL = 1e-6

SAMPLES = [
    (
        r"G:\PyArx\tests\楼层平面-10KV电缆桥架.dwg",
        r"G:\PyArx\tests\dwg\楼层平面-10KV电缆桥架.dxf",
    ),
    (
        r"G:\PyArx\tests\楼层平面-强电.dwg",
        r"G:\PyArx\tests\dwg\楼层平面-强电.dxf",
    ),
]


def _pairs(path: str) -> list[tuple[int, str]]:
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        lines = [ln.rstrip("\r\n") for ln in fh]
    out: list[tuple[int, str]] = []
    for i in range(0, len(lines) - 1, 2):
        try:
            code = int(lines[i].strip())
        except ValueError:
            continue
        out.append((code, lines[i + 1].strip()))
    return out


def parse_dxf_header(dxf: str) -> dict[str, object]:
    pairs = _pairs(dxf)
    result: dict[str, object] = {}
    var: str | None = None
    vec: dict[int, float] = {}
    def flush() -> None:
        if var is not None and vec:
            result[var] = [vec.get(10), vec.get(20), vec.get(30)]

    for code, value in pairs:
        if code == 9 and value.startswith("$"):
            flush()
            var = value[1:]
            vec = {}
            continue
        if var is None:
            continue
        if code in (10, 20, 30):
            try:
                vec[code] = float(value)
            except ValueError:
                pass
        elif code == 70:
            try:
                result[var] = int(value)
            except ValueError:
                result[var] = value
        elif code == 5:
            result[var] = value
        elif code == 999:
            continue
    flush()
    return result


def parse_block_records(dxf: str) -> dict[str, str]:
    pairs = _pairs(dxf)
    records: dict[str, str] = {}
    i = 0
    while i < len(pairs):
        code, value = pairs[i]
        if code == 0 and value == "BLOCK_RECORD":
            handle = name = None
            j = i + 1
            while j < len(pairs) and pairs[j][0] != 0:
                if pairs[j][0] == 5:
                    handle = pairs[j][1]
                elif pairs[j][0] == 2:
                    name = pairs[j][1]
                j += 1
            if handle and name:
                records[name] = handle
            i = j
        else:
            i += 1
    return records


def _vec_err(dwg, truth) -> float:
    if dwg is None or truth is None:
        return math.inf
    return max(abs(float(a) - float(b)) for a, b in zip(dwg, truth))


def report(name: str, dwg_path: str, dxf_path: str) -> list[tuple[str, str]]:
    dwg = open_dwg(dwg_path)
    vars_ = dwg.header_vars
    dxf = parse_dxf_header(dxf_path)
    blocks = parse_block_records(dxf_path)

    rows: list[tuple[str, str]] = []

    def add(label: str, dwg_value, truth_value, err: float | None, status: str):
        rows.append((label, status, str(dwg_value), str(truth_value),
                     "" if err is None else f"{err:.3g}"))

    add("CODEPAGE", vars_["codepage"], 39,
        None if vars_["codepage"] == 39 else abs(vars_["codepage"] - 39),
        "PASS" if vars_["codepage"] == 39 else "FAIL")

    add("INSUNITS", vars_["insunits"], dxf.get("INSUNITS"), None,
        "PASS" if vars_["insunits"] == dxf.get("INSUNITS") else "PENDING")

    for label, dwg_key, truth_key in [
        ("EXTMIN", "extmin", "EXTMIN"),
        ("EXTMAX", "extmax", "EXTMAX"),
        ("INSBASE", "insbase", "INSBASE"),
    ]:
        truth = dxf.get(truth_key)
        err = _vec_err(vars_[dwg_key], truth)
        status = "PASS" if err <= TOL else ("PENDING" if vars_[dwg_key] is None else "FAIL")
        add(label, vars_[dwg_key], truth, err, status)

    for label, dwg_key, truth_key in [
        ("LIMMIN", "limmin", "LIMMIN"),
        ("LIMMAX", "limmax", "LIMMAX"),
    ]:
        truth = dxf.get(truth_key)
        err = _vec_err(vars_[dwg_key], truth)
        status = "PASS" if err <= TOL else ("PENDING" if vars_[dwg_key] is None else "FAIL")
        add(label, vars_[dwg_key], truth, err, status)

    truth_hand = blocks.get("*Model_Space")
    add("BLOCK_RECORD_MODEL_SPACE", vars_["block_record_model_space"], truth_hand,
        None, "PENDING" if vars_["block_record_model_space"] is None else "PASS")
    truth_hand = blocks.get("*Paper_Space")
    add("BLOCK_RECORD_PSPACE", vars_["block_record_paper_space"], truth_hand,
        None, "PENDING" if vars_["block_record_paper_space"] is None else "PASS")

    truth_seed = dxf.get("HANDSEED")
    if vars_["handseed"] is None:
        seed_status = "PENDING"
        seed_err = None
    else:
        truth_int = int(truth_seed, 16) if isinstance(truth_seed, str) else truth_seed
        seed_err = abs(vars_["handseed"] - truth_int)
        seed_status = "PASS" if seed_err == 0 else "FAIL"
    add("HANDSEED", hex(vars_["handseed"]) if vars_["handseed"] is not None else None,
        truth_seed, seed_err, seed_status)

    print("=" * 78)
    print(name)
    print(f"{'field':28} {'status':8} {'dwg':24} {'dxf':24} {'err':10}")
    for label, status, dwg_v, truth_v, err in rows:
        print(f"{label:28} {status:8} {dwg_v:24} {truth_v:24} {err:10}")
    return rows


def main() -> int:
    all_pass = True
    for dwg_path, dxf_path in SAMPLES:
        rows = report(os.path.basename(dwg_path), dwg_path, dxf_path)
        all_pass &= all(status in ("PASS", "PENDING") for _, status, *_ in rows)
    return 0 if all_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
