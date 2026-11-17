"""Probe the AcDb:Classes section layout (R2007) for class-number -> name.

Follows ACadSharp DwgClassesReader.Read() step by step:
sentinel / RL size / RL flagOffset -> flagPos / BL 0 + B flag -> class records
with three TVs from the merged text stream.
"""
import sys

sys.path.insert(0, r"G:\PyArx")
from PyArxDwg.container import open_dwg, _locate_text_stream
from PyArxDwg.objects import Reader, _Bounds


def parse(data, start, last):
    text = _locate_text_stream(data, start, last)
    if not text:
        return None
    main = Reader(data, start, text[0])
    strs = Reader(data, text[0], text[1])
    recs = []
    try:
        main.bl()   # BL 0x00
        main.bit()  # B flag
        for _ in range(400):
            if main.pos >= main.end:
                break
            num = main.bs()
            pf = main.bs()
            app = strs.tv()
            cpp = strs.tv()
            dxf = strs.tv()
            main.bit()
            item = main.bs()
            for _ in range(5):
                main.bl()
            recs.append((num, dxf, cpp, item))
    except _Bounds:
        return None
    return main, strs, recs


def probe(path):
    c = open_dwg(path)
    data = c.section_bytes("AcDb:Classes")
    r = Reader(data, 16 * 8)
    size = r.rl()
    flag = r.rl()
    flag_pos = 160 + flag - 1
    print("size", size, "flag", flag, "flagPos", flag_pos,
          "len_bits", len(data) * 8, "start", r.pos)
    hits = []
    for last in range(max(r.pos, flag_pos - 64), flag_pos + 64):
        res = parse(data, r.pos, last)
        if not res:
            continue
        main, strs, recs = res
        if main.pos == main.end and recs:
            hits.append((last, len(recs), strs.pos, strs.end, main.pos, recs))
    print("closure hits", len(hits))
    for h in hits[:3]:
        print("  last", h[0], "recs", h[1], "strpos", h[2], h[3])
        for rec in h[5][:8]:
            print("     ", rec)


if len(sys.argv) > 1:
    probe(sys.argv[1])
else:
    probe(r"G:\PyArx\tests\楼层平面-10KV电缆桥架.dwg")
