"""M1 container-layer self-check for the two AC1021 samples.

Runs: python tests/dwg/check_container.py
Pure stdlib; does not read the parallel DXF/truth files.  Assertions:
  * required sections are listed by name;
  * every decoded section length equals its declared size (zero deviation);
  * pages tile [0, declared size) with no overlap and no hole;
  * AcDb:AcDbObjects and AcDb:Handles carry non-empty raw bytes.
"""

from __future__ import annotations

import os
import sys
import time

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

from PyArxDwg.container import open_dwg

HERE = os.path.dirname(os.path.abspath(__file__))
SAMPLES = [
    os.path.join(HERE, "..", "楼层平面-10KV电缆桥架.dwg"),
    os.path.join(HERE, "..", "楼层平面-强电.dwg"),
]

REQUIRED = (
    "AcDb:Header",
    "AcDb:Classes",
    "AcDb:Handles",
    "AcDb:AcDbObjects",
)


def check_sample(path: str) -> None:
    started = time.perf_counter()
    dwg = open_dwg(path)
    elapsed = time.perf_counter() - started

    missing = [name for name in REQUIRED if name not in dwg.sections]
    if missing:
        raise AssertionError(f"{os.path.basename(path)} missing sections: {missing}")

    print(f"\n=== {os.path.basename(path)}  ({os.path.getsize(path)} bytes)")
    print(f"version={dwg.version} maintenance={dwg.maintenance} codepage={dwg.codepage} "
          f"security={dwg.security_type} elapsed={elapsed:.3f}s")
    print(f"{'section':<22} {'declared':>10} {'decoded':>10} {'pages':>5} "
          f"{'compressed':>10} {'RS':>3}")

    for name, section in sorted(dwg.sections.items()):
        # strict length equality
        if len(section.data) != section.declared_size:
            raise AssertionError(
                f"{name}: decoded {len(section.data)} != declared {section.declared_size}"
            )

        # page tiling: sorted, contiguous, [0, size)
        pages = sorted(section.pages, key=lambda p: p.data_offset)
        cursor = 0
        for page in pages:
            if page.data_offset < cursor:
                raise AssertionError(f"{name}: page id {page.page_id} overlaps at 0x{cursor:X}")
            if page.data_offset > cursor:
                raise AssertionError(f"{name}: hole of {page.data_offset - cursor} bytes")
            if len(page.decoded) != page.uncompressed:
                raise AssertionError(
                    f"{name}: page id {page.page_id} length {len(page.decoded)} "
                    f"!= {page.uncompressed}"
                )
            cursor = page.data_offset + len(page.decoded)
        if cursor != section.declared_size:
            raise AssertionError(
                f"{name}: pages cover {cursor}, declared {section.declared_size}"
            )

        print(f"{name:<22} {section.declared_size:>10} {len(section.data):>10} "
              f"{section.page_count:>5} {str(section.compressed):>10} "
              f"{section.rs_corrected:>3}")

    if dwg.warnings:
        raise AssertionError(f"container warnings: {dwg.warnings}")

    for core in ("AcDb:AcDbObjects", "AcDb:Handles"):
        if not dwg.section_bytes(core):
            raise AssertionError(f"{core} is empty")

    if elapsed >= 2.0:
        raise AssertionError(f"container decode too slow: {elapsed:.3f}s")


def main() -> int:
    for sample in SAMPLES:
        check_sample(sample)
    print("\nALL M1 CONTAINER CHECKS PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
