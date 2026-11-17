"""多角度核验：对 79 台已识别柜逐台投票，看哪些特征一致、哪些矛盾。"""
import json
import re

SCENE = r"G:\PyArx\viewer\web\scene.json"
CAB = r"G:\PyArx\viewer\export\cabinets_all.json"
OUT = r"G:\PyArx\viewer\export\cab_verify.json"
YMIN = 500000
NAME = re.compile(r"\d{3}-\d+-\d{0,2}[A-Z]{2,}\d{0,2}")

d = json.load(open(SCENE, encoding="utf-8"))
cabs = json.load(open(CAB, encoding="utf-8"))["cabinets"]

L = d["ln"]
lines = [(L[i], L[i + 1], L[i + 2], L[i + 3], L[i + 4]) for i in range(0, len(L) - 5, 6)]
txt = [(t[0], t[1], t[2], t[6]) for t in d["tx"] if t[1] >= YMIN and (t[6] or "").strip()]
ins = [(b["x"], b["y"], b.get("k", "")) for b in d["ins"] if b.get("y", 0) >= YMIN]

TBX = [(t["b"][0] + t["b"][2]) / 2 for t in d["tb"]]
TBY = [(t["b"][1] + t["b"][3]) / 2 for t in d["tb"]]


def inb(x, y, b):
    return b[0] <= x <= b[2] and b[1] <= y <= b[3]


res = []
for c in cabs:
    b = c["box"]
    # 角度1 参数表：框内表数
    ntb = sum(1 for i in range(len(TBX)) if inb(TBX[i], TBY[i], b))
    # 角度2 柜名：格式 + 位置(表 x0 右侧 -3000~+9000)
    nm = [t for t in txt if inb(t[0], t[1], b) and NAME.search(t[3]) and 250 <= t[2] <= 340]
    nname = 1 if nm else 0
    # 角度3 柜内符号块
    blks = [i for i in ins if inb(i[0], i[1], b)]
    nblk = len(blks)
    # 角度4 图层：柜专用 CABLETRAY_DIM(li=3) / 电气系统(li=6)
    nlay = sum(1 for l in lines if inb((l[0] + l[2]) / 2, (l[1] + l[3]) / 2, b) and l[4] in (3, 6))
    # 角度5 关键文字：SPD / kW / 回路号 / 电缆规格
    lt = [t[3] for t in txt if inb(t[0], t[1], b)]
    nkw = sum(1 for s in lt if "SPD" in s or "kW" in s.upper() or "kA" in s or re.search(r"YJV|SC\d|BV-", s))
    # 角度6 图元归属唯一性
    tot = 0
    uni = 0
    for l in lines:
        mx, my = (l[0] + l[2]) / 2, (l[1] + l[3]) / 2
        if not inb(mx, my, b):
            continue
        tot += 1
        hit = sum(1 for o in cabs if inb(mx, my, o["box"]))
        if hit == 1:
            uni += 1
    ratio = round(uni / tot, 3) if tot else 0.0

    flags = {
        "表=1": ntb == 1,
        "有柜名": nname == 1,
        "有块": nblk >= 3,
        "专用图层": nlay >= 2,
        "关键文字": nkw >= 3,
        "归属唯一": ratio >= 0.9,
    }
    res.append({"i": c["i"] + 1, "name": c["name"], "box": b,
                "f": {"表数": ntb, "柜名": nname, "块": nblk, "专用层": nlay,
                      "关键文字": nkw, "唯一率": ratio},
                "ok": flags, "pass": sum(1 for v in flags.values() if v)})

print("角度通过率：")
for k in ("表=1", "有柜名", "有块", "专用图层", "关键文字", "归属唯一"):
    n = sum(1 for r in res if r["ok"][k])
    print("  %-6s %2d/79  %5.1f%%" % (k, n, 100.0 * n / len(res)))

print("\n全通过(6/6)：%d 台；5/6：%d 台；<=4：%d 台"
      % (sum(1 for r in res if r["pass"] == 6),
         sum(1 for r in res if r["pass"] == 5),
         sum(1 for r in res if r["pass"] <= 4)))
print("\n未全通过的台：")
for r in res:
    if r["pass"] < 6:
        bad = [k for k, v in r["ok"].items() if not v]
        print("  #%-3d %-18s 缺 %s   f=%s" % (r["i"], r["name"] or "—", "/".join(bad), r["f"]))
    if r["pass"] <= 3:
        pass

print("\n一致通过 6/6 的台（抽样 8 台）：")
shown = 0
for r in res:
    if r["pass"] == 6 and shown < 8:
        print("  #%-3d %-18s %s" % (r["i"], r["name"] or "—", r["f"]))
        shown += 1

json.dump(res, open(OUT, "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("\n写出", OUT)
