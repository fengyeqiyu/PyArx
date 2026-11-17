"""批量导出全部柜：以 79 张参数表为一台一表的锚点，切出区域，提柜名/参数/回路，带自检报警。"""
import html as H
import json
import os
import re

SCENE = r"G:\PyArx\viewer\web\scene.json"
OUTD = r"G:\PyArx\viewer\export"
YMIN = 500000
W, HH = 22000, 42000          # 表左下角起向右 / 向上展开
DW, DH = 2000, 2000           # 表本身再向外留边

CABNAME = re.compile(r"\d{3}-\d+-\d{0,2}[A-Z]{2,}\d{0,2}|^[A-Z]{2,}-?\d{1,2}-\d{1,2}$")
LOOPNO = re.compile(r"^(WP\d+|\d{3}-\d+(BL\d+)?-?\w*)$")

d = json.load(open(SCENE, encoding="utf-8"))

L = d["ln"]
lines = []
for i in range(0, len(L) - 5, 6):
    lines.append((L[i], L[i + 1], L[i + 2], L[i + 3]))

txt = []
for t in d["tx"]:
    s = (t[6] or "").strip()
    if s:
        txt.append((t[0], t[1], t[2], s))
pls = [p["p"] for p in d["pl"]]
crs = [(d["cr"][i], d["cr"][i + 1], d["cr"][i + 2]) for i in range(0, len(d["cr"]) - 4, 5)]
ins = [(b.get("x", 0), b.get("y", 0), b.get("k", "")) for b in d["ins"]]


def inb(x, y, b, pad=0):
    return b[0] - pad <= x <= b[2] + pad and b[1] - pad <= y <= b[3] + pad


def peek_tb(b):
    """区域内命中的参数表数量，>1 说明框到邻柜。"""
    n = 0
    for tb in d["tb"]:
        c = ((tb["b"][0] + tb["b"][2]) / 2, (tb["b"][1] + tb["b"][3]) / 2)
        if inb(c[0], c[1], b):
            n += 1
    return n


def grid(tb):
    t = tb.get("t") or []
    return [[str(c) if c is not None else "" for c in row] for row in t]


def pval(t, key):
    for row in t:
        if row and row[0].strip().rstrip("=").strip() == key:
            return row[1].strip() if len(row) > 1 else ""
    return ""


cabs = []
TB = d["tb"]
for ti, tb in enumerate(TB):
    bb = tb["b"]
    x1lim = bb[0] - DW + W + 2 * DW
    y1lim = bb[1] - DH + HH + 2 * DH
    for oi, ob in enumerate(TB):
        if oi == ti:
            continue
        ob = ob["b"]
        # 右侧邻表：x 在我右边、且纵向有重叠 -> 右界收到它左边
        if ob[0] > bb[2] and ob[0] - bb[0] < 32000 and not (ob[3] < bb[1] or ob[1] > bb[3]):
            x1lim = min(x1lim, ob[0] - 900)
        # 上方邻表：下沿在我上方、且横向有重叠 -> 上界收到它下沿
        if ob[1] > bb[3] and ob[1] - bb[3] < 48000 and not (ob[2] < bb[0] or ob[0] > bb[2]):
            y1lim = min(y1lim, ob[1] - 900)
    box = [round(bb[0] - DW), round(bb[1] - DH), round(x1lim), round(y1lim)]
    # 内容收紧：用框内文字的实际右界/上界替代固定 W/H（文字右端 = x + 字高×字数）
    seed = [x for x in txt if inb(x[0], x[1], box)]
    if seed:
        r_edge = max(x[0] + min(len(x[3]), 40) * x[2] for x in seed)
        t_edge = max(x[1] for x in seed)
        box[2] = min(round(max(r_edge + 1200, bb[2] + 1500)), box[2])
        box[3] = min(round(max(t_edge + 2600, bb[3] + 1500)), box[3])
        box[2] = max(box[2], round(bb[2] + 1200))
        box[3] = max(box[3], round(bb[3] + 1200))
    nmbox = [box[0] - 1000, box[1], box[2] + 3000, box[3] + 12000]
    t = grid(tb)
    pe, kx, cos, pjs, ijs = pval(t, "Pe"), pval(t, "Kx"), pval(t, "COS∅") or pval(t, "COS"), pval(t, "Pjs"), pval(t, "Ijs")

    txs = [x for x in txt if inb(x[0], x[1], box)]
    txs_nm = [x for x in txt if inb(x[0], x[1], nmbox)]
    name = ""
    cand = [x for x in txs_nm
            if abs(x[0] - bb[0]) < 4500 and bb[1] < x[1] < bb[1] + 50000 and CABNAME.search(x[3])]
    if cand:
        cand.sort(key=lambda v: abs(v[1] - (bb[1] + 37000)) + abs(v[0] - bb[0]) * 2)
        name = cand[0][3]
    loops, devices = [], []
    for x in txs:
        for tok in re.findall(r"W[A-Z]{0,2}-?\d+", x[3]):
            if tok not in loops:
                loops.append(tok)
        for tok in re.findall(r"(?:RAU|VRV|KT|CSJ|HU|MCU|FCU|FJ)[-\s]?\d+[-\s]?\d+", x[3]):
            t2 = tok.replace(" ", "")
            if t2 not in devices:
                devices.append(t2)
    ntb = peek_tb(box)
    warn = []
    if ntb > 1:
        warn.append("框内参数表 %d 张（串入邻柜）" % ntb)
    if not pe:
        warn.append("缺 Pe")
    if not ijs:
        warn.append("缺 Ijs")

    cabs.append({
        "i": ti, "name": name, "pe": pe, "kx": kx, "cos": cos, "pjs": pjs, "ijs": ijs,
        "box": box, "tables": ntb, "warn": "；".join(warn),
        "counts": {"lines": sum(1 for l in lines if inb(l[0], l[1], box) or inb(l[2], l[3], box)),
                   "texts": len(txs),
                   "plines": sum(1 for p in pls if p and inb(p[0], p[1], box)),
                   "circles": sum(1 for c in crs if inb(c[0], c[1], box)),
                   "blocks": sum(1 for b in ins if inb(b[0], b[1], box))},
        "grid": t,
        "loops": loops,
        "devices": devices,
        "texts": [[round(x[0]), round(x[1]), round(x[2]), x[3]] for x in sorted(txs, key=lambda v: -v[1])][:220],
    })

good = [c for c in cabs if not c["warn"]]
print("柜数 %d   干净 %d   报警 %d" % (len(cabs), len(good), len(cabs) - len(good)))
print("有柜名 %d   有 Pe %d   有 Ijs %d   有回路 %d"
      % (sum(1 for c in cabs if c["name"]), sum(1 for c in cabs if c["pe"]),
         sum(1 for c in cabs if c["ijs"]), sum(1 for c in cabs if c["loops"])))
for c in cabs[:6]:
    print("  #%d %-14s Pe=%-8s Ijs=%-8s 回路%3d 文本%3d 表%d %s"
          % (c["i"] + 1, c["name"] or "(无名)", c["pe"], c["ijs"], len(c["loops"]), c["counts"]["texts"],
             c["tables"], c["warn"]))

json.dump({"file": d["f"], "cabinets": cabs},
          open(os.path.join(OUTD, "cabinets_all.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)


# ---------- 汇总 HTML ----------
rows = []
for c in cabs:
    cls = "warn" if c["warn"] else ""
    lbl = "；".join(c["loops"][:6]) + ("…" if len(c["loops"]) > 6 else "")
    rows.append("<tr class='%s'><td>%d</td><td class='nm'>%s</td><td>%s</td><td>%s</td><td>%s</td>"
                "<td>%s</td><td>%s</td><td class='lp'>%s</td><td>%d</td><td>%d</td><td>%d</td>"
                "<td class='wg'>%s</td></tr>"
                % (cls, c["i"] + 1, H.escape(c["name"] or "—"), H.escape(c["pe"]), H.escape(c["kx"]),
                   H.escape(c["cos"]), H.escape(c["pjs"]), H.escape(c["ijs"]),
                   H.escape(lbl), len(c["loops"]), len(c["devices"]), c["counts"]["texts"],
                   H.escape(c["warn"]) or "—"))

head = ("柜号|柜名|Pe(kW)|Kx|COS|Pjs(kW)|Ijs(A)|回路号|回路数|设备数|文字数|自检").split("|")
html = """<!doctype html><html lang="zh-CN"><head><meta charset="utf-8">
<title>全部柜 · 批量结构化导出</title><style>
body{margin:0;background:#0f1214;color:#d7dee5;font:13px/1.6 "Microsoft YaHei",sans-serif}
.wrap{max-width:1240px;margin:24px auto;padding:0 16px}
h1{font-size:17px;color:#e9c46a;margin:0 0 6px}
.sub{color:#8b98a5;font-size:12px;margin-bottom:16px}
table{width:100%%;border-collapse:collapse;background:#15191c}
th,td{border-bottom:1px solid #232a30;padding:7px 9px;text-align:left;vertical-align:top}
th{background:#1b2125;color:#9fb0bd;font-weight:400;position:sticky;top:0}
tr.warn td{background:#241a17}
td.nm{color:#e9c46a}
td.wg{color:#e07a5f}
.tag{display:inline-block;padding:1px 7px;border-radius:9px;background:#1f2a1f;color:#8fc58f;font-size:11px;margin-left:8px}
</style></head><body><div class="wrap">
<h1>全部柜 · 批量结构化导出<span class="tag">__NCAB__ 台 / 干净 __NGOOD__</span></h1>
<div class="sub">__FILE__ · 以参数表为一台一表的锚点，向右 __W__ / 向上 __HH__ 取区域 · 红底行为自检报警（框内出现第二张参数表或参数缺失）</div>
<table><thead><tr>__HEAD__</tr></thead><tbody>
__ROWS__
</tbody></table></div></body></html>"""
html = (html.replace("__NCAB__", str(len(cabs))).replace("__NGOOD__", str(len(good)))
        .replace("__FILE__", H.escape(d["f"]))
        .replace("__W__", str(W)).replace("__HH__", str(HH))
        .replace("__HEAD__", "".join("<th>%s</th>" % h for h in head))
        .replace("__ROWS__", "\n".join(rows)))
open(os.path.join(OUTD, "cabinets_all.html"), "w", encoding="utf-8").write(html)
print("写出 cabinets_all.json / cabinets_all.html  %d KB"
      % (os.path.getsize(os.path.join(OUTD, "cabinets_all.html")) // 1024))
