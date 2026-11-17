# -*- coding: utf-8 -*-
"""
robustness.py —— 畸形输入健壮性 + 真实样本计时（纯标准库，子进程调解析器）。

六类畸形输入：
  截断：50% / 25% / 末 200 字节
  头部：版本改 AC9999；0x0B 起若干字节改 0xFF
  段映射：用解析器暴露的 page_mapping('AcDb:AcDbObjects') 定位真实页槽位，
          把该槽位字段改成超大值（0x00FFFFFF），并记录替换偏移作为证据。

判定（硬规则）：
  * 崩溃 = 负退出码 / Traceback / Fatal Python error / Access violation；
  * 可定位 = 非零退出且错误信息含文件偏移(0x..)、段名(AcDb:..)、头部标记或异常类型；
  * rc=0 即使“看起来容忍”也记 FAIL——除非该输入在语义上仍是合法 DWG（此处无此情形）；
  * 真实样本计时：大文件目标 < 2.0s（当前 rc=0 仅代表容器层解码成功）。

退出码：0 全部受控；1 存在崩溃/超时/被容忍的畸形输入；2 前置缺失。
"""
from __future__ import annotations

import argparse
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
import time
from typing import Any, Dict, List, Optional

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(os.path.dirname(HERE))
DEFAULT_DWGS = [
    os.path.join(REPO_ROOT, "tests", "楼层平面-10KV电缆桥架.dwg"),
    os.path.join(REPO_ROOT, "tests", "楼层平面-强电.dwg"),
]
BIG_THRESHOLD_S = 2.0
CRASH_MARKS = ("Traceback (most recent call last)", "Fatal Python error",
               "Access violation", "Segmentation fault")
OFFSET_RE = re.compile(r"0x[0-9A-Fa-f]+|offset\s+0x?[0-9A-Fa-f]+")
SECTION_RE = re.compile(r"AcDb:[A-Za-z]+|section\s+['\"][^'\"]+['\"]")
HEADER_RE = re.compile(r"magic|header|unsupported|too short|not a readable", re.I)
BIG_POINTER = 0x00FFFFFF
TARGET_SECTION = "AcDb:AcDbObjects"


def run_parser(path: str) -> Dict[str, Any]:
    env = os.environ.copy()
    env["PYTHONPATH"] = REPO_ROOT + os.pathsep + env.get("PYTHONPATH", "")
    t0 = time.perf_counter()
    proc = subprocess.run(
        [sys.executable, "-m", "PyArxDwg", "dump", path],
        capture_output=True, text=True, encoding="utf-8", errors="replace", env=env,
    )
    elapsed = time.perf_counter() - t0
    stderr = proc.stderr or ""
    stdout = proc.stdout or ""
    first_err = next((ln for ln in stderr.splitlines() if ln.strip()), "")
    crashed = proc.returncode < 0 or any(m in stderr for m in CRASH_MARKS)
    locatable = bool(
        OFFSET_RE.search(stderr) or SECTION_RE.search(stderr) or HEADER_RE.search(stderr)
        or re.search(r"Dwg(Format|Bounds)Error", stderr)
    )
    return {
        "rc": proc.returncode,
        "elapsed": elapsed,
        "stdout_tail": stdout.strip()[-400:],
        "stderr_tail": stderr.strip()[-400:],
        "stderr_first_line": first_err.strip(),
        "crashed": crashed,
        "locatable_error": locatable,
    }


def probe_page_slot(dwg_path: str) -> Optional[Dict[str, Any]]:
    """经解析器自身暴露的映射取某段首个真实页槽位的绝对文件偏移。"""
    sys.path.insert(0, REPO_ROOT)
    try:
        from PyArxDwg.container import open_dwg  # type: ignore
        container = open_dwg(dwg_path)
        pages = container.page_mapping(TARGET_SECTION)
        if not pages:
            return None
        data_offset, file_offset, compressed, uncompressed = pages[0]
        return {
            "data_offset": data_offset,
            "file_offset": file_offset,
            "compressed": compressed,
            "uncompressed": uncompressed,
        }
    except Exception as exc:  # 前置缺失：容器层未就绪
        return {"probe_error": f"{type(exc).__name__}: {exc}"}


def make_mutations(src: bytes, tag: str,
                   probe: Optional[Dict[str, Any]]) -> List[Dict[str, Any]]:
    n = len(src)
    out: List[Dict[str, Any]] = []

    def add(kind: str, note: str, data: bytes, evidence: Optional[Dict[str, Any]] = None) -> None:
        out.append({"kind": kind, "note": note, "data": data, "evidence": evidence or {}})

    add("truncate_50pct", f"截断到 {n // 2} 字节", src[: n // 2])
    add("truncate_25pct", f"截断到 {n // 4} 字节", src[: n // 4])
    add("truncate_drop_tail200", "删除末 200 字节", src[: max(0, n - 200)])
    add("magic_ac9999", "版本串改为 AC9999", b"AC9999" + src[6:])
    head = bytearray(src)
    head[0x0B:0x2B] = b"\xff" * min(0x20, n - 0x0B)
    add("header_ff_from_0x0B", "0x0B 起 32 字节置 0xFF", bytes(head))

    # 真实页槽位偏移 → 超大值；替换偏移与特征字节命中次数作为证据。
    if probe is None or "probe_error" in (probe or {}):
        add("section_offset_overlarge",
            "前置：容器层未暴露页槽位偏移，无法定位真实字段（未执行）", src,
            {"probe": probe})
        return out

    off = probe["file_offset"]
    feature = src[off: off + min(16, n - off)]
    hits = src.count(feature) if feature else 0
    big = bytearray(src)
    struct.pack_into("<Q", big, off, BIG_POINTER)
    add("section_offset_overlarge",
        f"{TARGET_SECTION} 首个页槽位 @0x{off:X} 偏移/字段改为 0x{BIG_POINTER:X}",
        bytes(big),
        {"patched_offset": f"0x{off:X}", "pattern_hits": hits,
         "data_offset": probe["data_offset"], "compressed": probe["compressed"],
         "uncompressed": probe["uncompressed"]})
    return out


def judge(res: Dict[str, Any]) -> str:
    """硬规则：崩溃 FAIL；rc=0 容忍 FAIL；非零且可定位 PASS；其余 FAIL。"""
    if res["crashed"]:
        return "fail"
    if res["rc"] == 0:
        return "fail"
    return "pass" if res["locatable_error"] else "fail"


def run_mutations(dwg_path: str, work_dir: str) -> Dict[str, Any]:
    with open(dwg_path, "rb") as fh:
        src = fh.read()
    base = os.path.splitext(os.path.basename(dwg_path))[0]
    probe = probe_page_slot(dwg_path)
    cases: List[Dict[str, Any]] = []
    for m in make_mutations(src, base, probe):
        if m["kind"] == "section_offset_overlarge" and "未执行" in m["note"]:
            cases.append({**{k: v for k, v in m.items() if k != "data"},
                          "status": "not_executed"})
            continue
        fpath = os.path.join(work_dir, f"mut_{base}_{m['kind']}.dwg")
        with open(fpath, "wb") as fh:
            fh.write(m["data"])
        res = run_parser(fpath)
        cases.append({
            "kind": m["kind"], "note": m["note"], "bytes": len(m["data"]),
            "evidence": m["evidence"], "status": judge(res), **res,
        })
    return {"dwg": dwg_path, "cases": cases}


def time_real_samples(dwgs: List[str]) -> List[Dict[str, Any]]:
    out: List[Dict[str, Any]] = []
    for dwg in dwgs:
        if not os.path.exists(dwg):
            out.append({"dwg": dwg, "status": "prerequisite", "reason": "DWG 不存在"})
            continue
        res = run_parser(dwg)
        size = os.path.getsize(dwg)
        succeeded = res["rc"] == 0
        over = res["elapsed"] > BIG_THRESHOLD_S
        status = "pass" if succeeded and not over else ("fail" if over else "blocked")
        out.append({
            "dwg": dwg, "size": size, "elapsed": res["elapsed"],
            "rc": res["rc"], "succeeded": succeeded,
            "threshold_s": BIG_THRESHOLD_S, "over_threshold": over,
            "status": status,
            "note": ("解析未成功，耗时仅为失败耗时，不做性能达标判定" if not succeeded
                     else "当前 rc=0 仅代表容器层解码成功；对象/实体层耗时尚不可测"),
            "stderr_first_line": res["stderr_first_line"],
        })
    return out


def main(argv: Optional[List[str]] = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dwgs", nargs="*")
    ap.add_argument("--out", default=HERE)
    args = ap.parse_args(argv)
    dwgs = args.dwgs or DEFAULT_DWGS

    with tempfile.TemporaryDirectory(prefix="dwg_mut_", dir=HERE) as work:
        mutation_results = [run_mutations(d, work) for d in dwgs if os.path.exists(d)]
    timing = time_real_samples(dwgs)

    rep = {"mutation_threshold_s": BIG_THRESHOLD_S,
           "mutations": mutation_results, "timing": timing}
    os.makedirs(args.out, exist_ok=True)
    with open(os.path.join(args.out, "robustness_report.json"), "wb") as fh:
        fh.write(json.dumps(rep, ensure_ascii=False, indent=1).encode("utf-8"))

    n_fail = 0
    for mr in mutation_results:
        for c in mr["cases"]:
            if c.get("status") != "not_executed" and c["status"] == "fail":
                n_fail += 1
    n_over = sum(1 for t in timing if t.get("over_threshold"))
    blocked = all(t.get("status") == "blocked" for t in timing)

    print("== 畸形输入 ==")
    for mr in mutation_results:
        print(f"- {os.path.basename(mr['dwg'])}")
        for c in mr["cases"]:
            ev = c.get("evidence") or {}
            ev_txt = f" patch=0x{int(ev.get('patched_offset', '0'), 16):X}" if ev.get("patched_offset") else ""
            print(f"  [{c['status']:<12}] {c['kind']:<28} rc={c.get('rc')} "
                  f"crash={c.get('crashed')} locatable={c.get('locatable_error')} "
                  f"elapsed={c.get('elapsed', 0):.3f}s{ev_txt}")
            if c.get("stderr_first_line"):
                print(f"      首行: {c['stderr_first_line']}")
    print("\n== 真实样本计时 ==")
    for t in timing:
        print(f"- {os.path.basename(t['dwg'])} size={t.get('size')} "
              f"elapsed={t.get('elapsed', 0):.3f}s rc={t.get('rc')} status={t['status']} "
              f"{t.get('note', '')}")

    if not mutation_results and blocked:
        code = 2
    elif n_fail or n_over:
        code = 1
    else:
        code = 0
    print(f"\nrobustness exit={code}")
    return code


if __name__ == "__main__":
    sys.exit(main())
