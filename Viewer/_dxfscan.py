# -*- coding: utf-8 -*-
"""在 DXF 组码里读 _AXISO 块定义，看块内到底有什么实体、文字存在哪。"""
DXF = r"C:\temp\101_ax.dxf"
KEYS = ("1", "2", "3", "8", "62", "66", "70", "92", "310", "10", "20", "30", "40", "50", "41")


def pairs(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        pend = None
        for line in f:
            s = line.strip()
            if pend is None:
                if s:
                    pend = s
                continue
            yield pend, line.rstrip("\r\n")
            pend = None


out = {}
open_blk = None
records = []
cur = None
global_types = {}

for code, val in pairs(DXF):
    code = code.strip()
    if code == "0":
        ty = val.strip()
        if ty == "BLOCK":
            open_blk = "?"
            records = []
            cur = None
            continue
        if open_blk:
            if ty == "ENDBLK":
                if cur:
                    records.append(cur)
                if "_AXISO" in str(open_blk):
                    out.setdefault(open_blk, []).extend(records)
                open_blk = None
                cur = None
                continue
            if cur:
                records.append(cur)
            cur = {"t": ty, "g": []}
        continue
    if open_blk:
        if code == "2" and open_blk == "?":
            open_blk = val.strip()
            continue
        if cur is not None:
            cur["g"].append((code, val.strip()))

print("== _AXISO blocks:", len(out))
for name, ents in out.items():
    print("BLOCK", name, "entities", len(ents))
    for e in ents:
        pick = [(c, v[:46]) for c, v in e["g"] if c in KEYS]
        print("   ", e["t"], pick)

# 全图统计：ATTDEF / ATTRIB 有多少，分别有没有 1 组码文字
cnt = {}
with_text = {}
for code, val in pairs(DXF):
    code = code.strip()
    if code == "0":
        ty = val.strip()
        if ty in ("ATTDEF", "ATTRIB", "TEXT", "MTEXT"):
            cnt[ty] = cnt.get(ty, 0) + 1
        continue
    if code == "1" and ty in ("ATTDEF", "ATTRIB", "TEXT", "MTEXT"):
        with_text[ty] = with_text.get(ty, 0) + 1
print("types", cnt, "withCode1", with_text)
