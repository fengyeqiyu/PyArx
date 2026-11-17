import json, PyArxCore as P

out = {}
db = P.Db.curDb()

def pairs(data):
    """entget 返回扁平数组：code,value,code,value..."""
    lst = list(data)
    res = []
    i = 0
    while i + 1 < len(lst):
        code = lst[i]
        val = lst[i + 1]
        # 点值可能是 (x,y,z) 三元组或连续三个数
        if code in (10, 11, 210) and isinstance(val, (int, float)) and i + 3 < len(lst):
            res.append((code, (float(lst[i + 1]), float(lst[i + 2]), float(lst[i + 3]))))
            i += 4
            continue
        res.append((code, val))
        i += 2
    return res

stats = {}
samples = {}
ss = P.Ed.ssget("X", None, None, None)
n = P.Ed.sslength(ss)
for i in range(n):
    nm = P.Ed.ssname(ss, i)
    data = pairs(P.Ed.entget(nm))
    typ = str(dict(data).get(0, "?"))
    lay = str(dict(data).get(8, ""))
    stats.setdefault(lay, {}).setdefault(typ, 0)
    stats[lay][typ] += 1
    if typ not in samples:
        samples[typ] = [[c, str(v)] for c, v in data]

out["stats"] = stats
out["samples"] = samples
json.dump(out, open(r"G:\PyArx\log\tray_probe2.json", "w", encoding="utf-8"), ensure_ascii=False, default=str)
