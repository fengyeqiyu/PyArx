"""Dump every registered enum in PyArxCore (module -> enum -> {member: value}) to JSON."""
import json
import PyArxCore as C

out = {}
for modname in dir(C):
    mod = getattr(C, modname)
    if type(mod).__name__ != "module":
        continue
    enums = {}
    for name in dir(mod):
        obj = getattr(mod, name)
        if not isinstance(obj, type):
            continue  # export_values 出来的枚举实例不算枚举类型
        members = getattr(obj, "__members__", None)
        if not isinstance(members, dict):
            continue
        vals = {}
        for k, v in members.items():
            for getter in (lambda x: int(x), lambda x: int(x.value)):
                try:
                    vals[k] = getter(v)
                    break
                except Exception:
                    continue
            else:
                vals[k] = None
        enums[name] = vals
    out[modname] = enums

with open(r"G:/PyArx/log/enum_dump.json", "w") as f:
    json.dump(out, f)
tot = sum(len(v) for v in out.values())
mem = sum(len(mm) for v in out.values() for mm in v.values())
print("[ENUM] modules=%d enums=%d members=%d" % (len(out), tot, mem))
