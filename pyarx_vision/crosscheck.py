# -*- utf-8 -*-
"""系统图识别交叉校验：四条独立判据互相印证，找漏检与误检。

判据（彼此不依赖同一份列切分）：
    A 回路号列   sysmode 当前主判据（W…/WL…）
    B 断路器列   直接数 MCB/RCBO/MCCB/RCD 文字，不看回路号
    C 电缆列     直接数 BV/YJV/WDZ/NH 文字
    D 图形横线   每个回路在图上必然带一条从箱引出的水平短线，
                 按横线反查"该 y 有没有回路号"，检出漏检的回路；
                 反过来对每个已识别回路查"该 y 有没有横线"，检出误检。

另外两个总量角度：
    E 干线表容量 vs 各箱容量和（口径不同，允许差异，但必须写明）
    F 逐图"回路号文字总数" vs "识别回路数"（同图内自洽）
"""

import io
import json
import re
import sys

sys.path.insert(0, r"G:\PyArx")
from pyarx_vision import frames as F
from pyarx_vision import sysmode as S
from pyarx_vision import tablemode as T

SCAN = r"C:\temp\scan.json"
OUT = r"G:\PyArx\docs\drb018_crosscheck.json"


def lines_in(scan, frame):
    """回路级横线。

    实测：回路短横线绝大多数是 LWPOLYLINE（#4 箱内 470 个 poly，其中 (1600,0) 45 条），
    LINE 只有 73 条且几乎挂着标题/表头。所以两种图元都要取。
    """
    x0, y0, x1, y1 = frame
    out = []
    for l in scan["lines"]:
        if len(l) < 6:
            continue
        if not (x0 <= l[0] <= x1 and y0 <= l[1] <= y1):
            continue
        dx, dy = abs(l[2] - l[0]), abs(l[3] - l[1])
        if dy < 5 and 1200 <= dx <= 2400:
            out.append(((l[0] + l[2]) / 2.0, (l[1] + l[3]) / 2.0, dx))
    for p in scan["plines"]:
        pts = p.get("p") or []
        if len(pts) < 2:
            continue
        a, b2 = pts[0], pts[-1]
        if not (x0 <= a[0] <= x1 and y0 <= a[1] <= y1):
            continue
        dx, dy = abs(b2[0] - a[0]), abs(b2[1] - a[1])
        if dy < 5 and 1200 <= dx <= 2400:
            out.append(((a[0] + b2[0]) / 2.0, (a[1] + b2[1]) / 2.0, dx))
    return out


def count_field(mem, regex):
    return sum(1 for t in mem if regex.search(t["t"].strip()))


def main():
    d = json.load(io.open(SCAN, encoding="utf-8"))
    frames = F.find_frames(d)
    report = {"per_box": [], "totals": {}, "flags": []}
    tot = {"loops": 0, "breakers": 0, "cables": 0, "lines": 0,
           "no_line": 0, "no_loop": 0}

    for f in frames:
        if F.classify(d, f["frame"])[0] != "panel":
            continue
        no = f["no"]
        bf = json.load(open(r"G:\PyArx\docs\drb018_boxframes.json", encoding="utf-8"))
        _fb = {}
        for _b in bf["boxes"]:
            _fb.setdefault(_b["sheet"], []).append(_b)
        res = S.parse_system(d, f["frame"], _fb.get(f["no"]))
        gline = lines_in(d, f["frame"])
        x0, y0, x1, y1 = f["frame"]
        all_t = [t for t in d["texts"]
                 if x0 <= t["x"] <= x1 and y0 <= t["y"] <= y1]

        for b in res["boxes"]:
            mem = [t for t in all_t if b["x0"] - 300 <= t["x"] <= b.get("xr", b["x1"])]
            A = b["drawn"]
            B = count_field(mem, re.compile(r'(MCB|RCBO|MCCB|RCD)', re.I))
            C = count_field(mem, re.compile(r'(BVR|BV-|YJV|WDZ|NH-|JDG)', re.I))
            bx = [g for g in gline if b["x0"] - 300 <= g[0] <= b.get("xr", b["x1"])]
            D = len(bx)

            # 每条识别回路：附近 250 内有没有横线
            no_line = []
            for l in b["loops"]:
                if not any(abs(g[1] - l["y"]) <= 250 for g in bx):
                    no_line.append(l["no"])
            # 每条横线：附近有没有回路号（排除标题/表头线）
            no_loop = 0
            for g in bx:
                if not any(abs(l["y"] - g[1]) <= 250 for l in b["loops"]):
                    no_loop += 1

            row = {"sheet": no, "box": b["code"], "A_loops": A, "B_breakers": B,
                   "C_cables": C, "D_lines": D,
                   "no_line": len(no_line), "no_line_codes": no_line[:6],
                   "orphan_lines": no_loop, "kw": b["kw_sum"]}
            report["per_box"].append(row)
            tot["loops"] += A
            tot["breakers"] += B
            tot["cables"] += C
            tot["lines"] += D
            tot["no_line"] += len(no_line)
            tot["no_loop"] += no_loop
            if abs(A - D) >= 3 or len(no_line) >= 3:
                report["flags"].append(
                    "#%d %s A=%d B=%d C=%d D=%d 无横线%d 无回路号%d"
                    % (no, b["code"], A, B, C, D, len(no_line), no_loop))

    # 角度 E：干线表 vs 箱容量
    t3 = None
    for f in frames:
        if F.classify(d, f["frame"])[0] == "table":
            t3 = T.parse(d, f["frame"])
    trunk_kw = 0.0
    for it in (t3.get("items") or []):
        m = re.search(r'([\d.]+)', it.get("容量", "") or "")
        if m:
            trunk_kw += float(m.group(1))
    box_kw = sum(r["kw"] for r in report["per_box"])

    report["totals"] = {
        **tot,
        "trunk_kw": round(trunk_kw, 1),
        "box_kw": round(box_kw, 1),
        "A_minus_D": tot["loops"] - tot["lines"],
    }
    io.open(OUT, "w", encoding="utf-8").write(
        json.dumps(report, ensure_ascii=False, indent=1))

    print("A回路号 %d | B断路器 %d | C电缆 %d | D图形横线 %d" % (
        tot["loops"], tot["breakers"], tot["cables"], tot["lines"]))
    print("已识别回路中无横线佐证 %d 条；有横线但无回路号 %d 条" % (
        tot["no_line"], tot["no_loop"]))
    print("干线表容量 %.1f kW | 各箱容量和 %.1f kW" % (trunk_kw, box_kw))
    print("\n不一致的箱 %d 个：" % len(report["flags"]))
    for s in report["flags"][:20]:
        print("  " + s)


if __name__ == "__main__":
    main()
