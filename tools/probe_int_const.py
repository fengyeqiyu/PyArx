import io, PyArxCore as C
INT_MIN, INT_MAX = -2147483648, 2147483647
mods = [("Db", C.Db), ("Ge", C.Ge), ("Gi", C.Gi), ("Ed", C.Ed), ("Ap", C.Ap), ("Rx", C.Rx)]
bad = []
cnt = 0
for mn, m in mods:
    for n in dir(m):
        if n.startswith("_"):
            continue
        v = getattr(m, n)
        if isinstance(v, bool):
            continue
        if isinstance(v, int):
            cnt += 1
            if v < INT_MIN or v > INT_MAX:
                bad.append("%s.%s=%d" % (mn, n, v))
print("[INTCONST] plain int attrs=%d out_of_int_range=%d" % (cnt, len(bad)))
for b in bad[:20]:
    print("[INTCONST] " + b)