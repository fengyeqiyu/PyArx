# -*- coding: utf-8 -*-
"""Minimal PDF text extraction for the local ODA spec (FlateDecode + Tj/TJ)."""
from __future__ import annotations

import re
import sys
import zlib

PDF = r"G:\PyArx\tests\dwg\oda_spec.pdf"


def streams(data: bytes):
    for m in re.finditer(rb"stream\r?\n", data):
        start = m.end()
        end = data.find(b"endstream", start)
        if end < 0:
            continue
        raw = data[start:end]
        try:
            yield zlib.decompress(raw)
        except Exception:
            continue


def page_text(blob: bytes) -> str:
    txt = []
    for m in re.finditer(rb"\((?:\\.|[^()\\])*\)", blob):
        s = m.group(0)[1:-1]
        s = s.replace(b"\\(", b"(").replace(b"\\)", b")").replace(b"\\\\", b"\\")
        txt.append(s)
    out = b" ".join(txt).decode("latin-1", errors="replace")
    return re.sub(r"\s+", " ", out)


def main(argv):
    needle = argv[1] if len(argv) > 1 else "string stream"
    data = open(PDF, "rb").read()
    pages = [page_text(s) for s in streams(data)]
    hits = 0
    for i, t in enumerate(pages):
        low = t.lower()
        pos = 0
        while True:
            k = low.find(needle.lower(), pos)
            if k < 0:
                break
            hits += 1
            print(f"\n=== chunk {i} @ {k} ===")
            print(t[max(0, k - 400): k + 900])
            pos = k + len(needle)
            if hits > 40:
                return 0
    print(f"hits={hits} chunks={len(pages)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
