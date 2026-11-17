import io, PyArxCore as C
MODS=[("Db",C.Db),("Ge",C.Ge),("Gi",C.Gi),("Ed",C.Ed),("Ap",C.Ap),("Rx",C.Rx)]
qual=set()
for mn,m in MODS:
    for n in dir(m):
        if n.startswith("_"): continue
        o=getattr(m,n)
        if isinstance(getattr(o,"__members__",None),dict) and o.__members__:
            qual.add("%s.%s.%s" % (C.__name__, mn, n))
sites=[]
for mn,m in MODS:
    for n in dir(m):
        if n.startswith("_"): continue
        o=getattr(m,n)
        docs=[]
        if isinstance(o,type):
            for a in dir(o):
                if a.startswith("_"): continue
                try: sub=getattr(o,a)
                except Exception: continue
                d=getattr(sub,"__doc__","") or ""
                if d: docs.append(("%s.%s.%s"%(mn,n,a),d))
        else:
            d=getattr(o,"__doc__","") or ""
            if d: docs.append(("%s.%s"%(mn,n),d))
        for w,d in docs:
            for line in d.splitlines():
                low=line.strip()
                if any(q in low for q in qual):
                    sites.append((w,low[:150]))
io.open(r"G:\PyArx\log\enum_params_runtime2.txt","w",encoding="utf-8").write("\n".join("%-50s | %s"%(w,s) for w,s in sites))
print("[EP2] enumTypes=%d enumTypedParamSites=%d" % (len(qual), len(sites)))