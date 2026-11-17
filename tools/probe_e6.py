import PyArxCore as C

Db, Ed = C.Db, C.Ed
for mod, label in ((Ed, "Ed"), (Db, "Db")):
    names = [n for n in dir(mod) if not n.startswith("_")]
    print("[E6] %s count=%d" % (label, len(names)))
    print("[E6] %s sel/print names: %s" % (label, ",".join(n for n in names if "elect" in n or "ssget" in n.lower() or "lot" in n)))
print("[E6] Ed.ssget=%s Ed.SelectionSet=%s" % (hasattr(Ed, "ssget"), hasattr(Ed, "SelectionSet")))
print("[E6] Db.HostApplicationServices=%s" % hasattr(Db, "HostApplicationServices"))
if hasattr(Db, "HostApplicationServices"):
    print("[E6] HApp members: %s" % ",".join(n for n in dir(Db.HostApplicationServices) if "lot" in n))
print("[E6] Ap.HostApplicationServices=%s" % hasattr(C.Ap, "HostApplicationServices"))
print("[E6] Db.IdPair=%s" % hasattr(Db, "IdPair"))
