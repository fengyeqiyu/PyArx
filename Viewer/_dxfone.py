# -*- coding: utf-8 -*-
"""按组码对顺序dump：找到 2=_AXISO 的 INSERT，打印其后 130 行原始组码。"""
DXF = r"C:\temp\101_ax.dxf"


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


last_type = ""
collect = 0
shown = 0
for c, v in pairs(DXF):
    if collect > 0:
        print("  %3s %s" % (c, v[:56]))
        collect -= 1
        continue
    if c == "0":
        last_type = v.strip()
        continue
    if c == "2" and "_AXISO" in v and last_type == "INSERT":
        print("===== INSERT blk=%s" % v.strip())
        collect = 130
        shown += 1
        if shown >= 2:
            pass
    if shown >= 2 and collect == 0:
        break
