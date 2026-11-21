# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity
from bitio import BitStream

REF = r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt"
m = {}
for line in open(REF, encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4:
        m[int(p[0])] = (int(p[1]), p[3])

DATA = entity.DATA

def dump(bh):
    addr, nm = m[bh]
    b = BitStream(DATA); b.byte = addr
    tc, bitsize, handle = entity.read_header(b)
    print("=== bh", bh, "addr", addr, "type", tc, "bitsize", bitsize)
    entity.decode_eed(b)
    num_reactors = b.read_bl()
    xdic_missing = b.read_b()
    print("reactors", num_reactors, "xdic_missing", xdic_missing)
    name = b.read_tv()
    print("name", name)
    resolved = b.read_bs()
    dep = 1 if resolved == 256 else 0
    _, _, xref = entity.read_h(b)
    print("resolved", resolved, "dep", dep, "xref", xref)
    anon = b.read_b(); hasattrs = b.read_b(); xref = b.read_b(); xrefov = b.read_b()
    xref_loaded = b.read_b()
    print("anon", anon, "hasattrs", hasattrs, "xref", xref, "xrefov", xrefov, "loaded", xref_loaded)
    num_owned = b.read_bl() if not (xref or xrefov) else 0
    print("num_owned", num_owned)
    base = b.read_3bd()
    print("base_pt", base)
    xpname = b.read_tv()
    print("xref_pname", xpname)
    num_ins = b.read_rl()
    print("num_inserts", num_ins)
    desc = b.read_tv()
    print("description", desc)
    pvsz = b.read_bl()
    print("preview_size", pvsz)
    if pvsz > 0:
        b.read_tf(pvsz)
    h = entity.open_handles(addr, bitsize)
    _, _, owner = entity.read_h(h)
    reactors = [entity.read_h(h)[2] for _ in range(num_reactors)]
    xdic = 0
    if not xdic_missing:
        _, _, xdic = entity.read_h(h)
    _, _, block_entity = entity.read_h(h)
    ents = [entity.read_h(h)[2] for _ in range(num_owned)]
    _, _, endblk = entity.read_h(h)
    inserts = [entity.read_h(h)[2] for _ in range(num_ins)]
    _, _, layout = entity.read_h(h)
    print("owner", owner, "reactors", reactors, "xdic", xdic)
    print("block_entity", block_entity, "endblk", endblk, "layout", layout)
    print("entities", len(ents), ents[:10])
    print("inserts", len(inserts), inserts[:5])
    return ents

for bh in (22, 79, 202):
    try:
        dump(bh)
    except Exception as e:
        import traceback; traceback.print_exc()
