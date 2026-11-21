# -*- coding: utf-8 -*-
import json, io
p = r"G:\PyArx\viewer\web\scene.json"
d = json.load(io.open(p, encoding="utf-8"))
print("keys", list(d.keys()))
print("f", d.get("f"), "n", d.get("n"), "b", d.get("b"), "cb", d.get("cb"), "v", d.get("v"))
print("c len", len(d["c"]), "head", d["c"][:6])
print("ly", d["ly"][:6], "lyc", d["lyc"][:6], "lyh", d["lyh"][:6])
for k in ("ln","cr","ar","pl","tx","ins","blk","tb"):
    v = d.get(k)
    if isinstance(v, list): print(k, "list", len(v), "sample", v[:2])
    elif isinstance(v, dict): print(k, "dict", len(v), "sample keys", list(v.keys())[:3])
    else: print(k, type(v))
