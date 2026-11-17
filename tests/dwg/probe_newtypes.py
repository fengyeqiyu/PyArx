# -*- coding: utf-8 -*-
"""Compare parser output with truth for the newly decoded entity types."""
from __future__ import annotations

import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
DWG = os.path.join(REPO, "tests", "楼层平面-强电.dwg")
TRUTH = os.path.join(HERE, "truth_楼层平面-强电.json")
EXE = os.path.join(REPO, "X64", "Release", "python.exe")
TYPES = ("ELLIPSE", "HATCH", "SPLINE", "POLYLINE", "VERTEX", "POINT", "3DFACE", "SOLID", "TRACE")


def main() -> int:
    env = dict(os.environ, PYTHONPATH=REPO)
    out = subprocess.run(
        [EXE, "-m", "PyArxDwg", "dump", DWG, "--json"],
        capture_output=True, text=True, encoding="utf-8", env=env,
    )
    if out.returncode != 0:
        print(out.stderr[-800:])
        return 1
    parser = json.loads(out.stdout)
    pent = {e["handle"].upper(): e for e in parser["entities"]}
    for b in parser["blocks"]:
        for e in b.get("entities", []):
            pent.setdefault(e["handle"].upper(), e)
    truth = json.load(open(TRUTH, encoding="utf-8"))
    tent = {e["handle"].upper(): e for e in truth["entities"] if e["type"] in TYPES}

    stats = {}
    mismatch = []
    for h, te in tent.items():
        pe = pent.get(h)
        ty = te["type"]
        stats[ty] = stats.get(ty, 0) + 1
        if pe is None:
            mismatch.append((h, ty, "missing"))
            continue
        tg, pg = te.get("geometry") or {}, pe.get("geometry") or {}
        for key in ("pattern", "flags", "degree", "major_ratio"):
            if key in tg and tg[key] != pg.get(key):
                mismatch.append((h, ty, key, tg[key], pg.get(key)))
        for key in ("center", "major_axis", "location"):
            if key in tg:
                tv, pv = tg[key], pg.get(key)
                if not isinstance(pv, list) or len(pv) != len(tv) or any(
                    abs(float(a) - float(b)) > 1e-6 for a, b in zip(tv, pv)
                ):
                    mismatch.append((h, ty, key, tv, pv))
    print("truth types:", stats)
    print("mismatches:", len(mismatch))
    for m in mismatch[:15]:
        print("  ", m)
    return 0


if __name__ == "__main__":
    sys.exit(main())
