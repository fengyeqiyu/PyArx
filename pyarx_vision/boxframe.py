# -*- utf-8 -*-
"""boxframe —— 先认柜体矩形框，记下坐标。

系统图里每台柜子都是一个矩形框：左右两条竖直长线，长度就是柜高。
（实测 #4：左列 x=514280/523251，各有 25900 与 29500 两条——上下两台柜；
  右列 x=549715/558686 同理。）

识别步骤：
  1. 取本图框内、坐标落在图框范围的竖直线段（块内局部坐标会被坐标范围挡掉）
  2. 按 x 聚类成"竖直边界"，相邻两条边界（间距 6~15 米）构成一台柜的宽度
  3. 同一对边界上，按"长度相同 + 端点相同"把左右竖线配对 → 一个矩形 = 一台柜
  4. 框内至少要认到 3 个回路号才算柜
  5. 柜名取框内最靠上的 101-1-1XXX

产物：drb018_boxframes.json —— 每台柜的 x0/y0/x1/y1，供回路归属与裁切渲染使用。
"""

import json
import os
import re
import sys

sys.path.insert(0, r"G:\PyArx")
from pyarx_vision import frames as F

OUT = r"G:\PyArx\docs\drb018_boxframes.json"
RE_LOOPNO = re.compile(r"^(WL|WP|WF|WC|WX|W[A-Z]?|AL|AP|L|P)[-\s]*\d{1,3}$")
RE_CODE = re.compile(r'^\d{2,}[-—]\d[0-9A-Za-z\-—:]{2,14}$')


def vsegs(d, frame):
    """图框内、落在图框坐标范围内的竖直线段：[(x, y0, y1)]"""
    X0, Y0, X1, Y1 = frame
    out = []
    pts = []
    for p in d.get("plines", []):
        for q in (p.get("p") or [])[:1] + (p.get("p") or [])[-1:]:
            pts.append(q)
        pp = p.get("p") or []
        if len(pp) >= 2:
            out.append((pp[0], pp[-1]))
    for l in d.get("lines", []):
        if len(l) >= 6:
            out.append(((l[0], l[1]), (l[2], l[3])))
    res = []
    for a, c in out:
        if not (X0 <= a[0] <= X1 and Y0 <= a[1] <= Y1):
            continue
        if not (X0 <= c[0] <= X1 and Y0 <= c[1] <= Y1):
            continue
        if abs(a[0] - c[0]) >= 3:
            continue
        y0, y1 = min(a[1], c[1]), max(a[1], c[1])
        if y1 - y0 < 5500 or y1 - y0 > 56000:
            continue
        res.append((a[0], y0, y1))
    return res


def cluster_x(vs, tol=300):
    vs = sorted(vs, key=lambda v: v[0])
    g = []
    for v in vs:
        if g and v[0] - g[-1][-1][0] <= tol:
            g[-1].append(v)
        else:
            g.append([v])
    return g


def find_frames(d):
    out = []
    for f in F.find_frames(d):
        if F.classify(d, f["frame"])[0] != "panel":
            continue
        X0, Y0, X1, Y1 = f["frame"]
        vs = vsegs(d, f["frame"])
        groups = cluster_x(vs)
        xs = [sum(v[0] for v in g) / len(g) for g in groups]
        txt = [t for t in d["texts"] if X0 <= t["x"] <= X1 and Y0 <= t["y"] <= Y1]
        boxes = []
        for i in range(len(groups)):
            for j in range(i + 1, len(groups)):
                wgap = xs[j] - xs[i]
                if not (4000 <= wgap <= 17000):
                    continue
                # 左右竖线按端点配对：长度接近、上下端接近 → 同一台柜
                for lv in groups[i]:
                    for rv in groups[j]:
                        if abs((lv[2] - lv[1]) - (rv[2] - rv[1])) > 900:
                            continue
                        if abs(lv[1] - rv[1]) > 1200 or abs(lv[2] - rv[2]) > 1200:
                            continue
                        x0, x1 = xs[i], xs[j]
                        y0, y1 = min(lv[1], rv[1]), max(lv[2], rv[2])
                        mem = [t for t in txt
                               if x0 - 200 <= t["x"] <= x1 + 200
                               and y0 - 200 <= t["y"] <= y1 + 200]
                        loops = [t for t in mem
                                 if RE_LOOPNO.match(t["t"].strip())
                                 and len(t["t"].strip()) <= 8
                                 and x0 - 1200 < t["x"] < x1 + 1600]
                        if len(loops) < 3:
                            continue
                        code = ""
                        for t in sorted(mem, key=lambda t: -t["y"]):
                            if RE_CODE.match(t["t"].strip()) and ":" not in t["t"]:
                                code = t["t"].strip()
                                break
                        boxes.append({
                            "sheet": f["no"], "code": code,
                            "x0": round(x0), "y0": round(y0),
                            "x1": round(x1), "y1": round(y1),
                            "loopnos": len(loops),
                        })
        # 同框重叠的去掉小的（内层标注框）
        keep = []
        for b in sorted(boxes, key=lambda b: -(b["x1"] - b["x0"]) * (b["y1"] - b["y0"])):
            if all(not (b["x0"] < k["x1"] and k["x0"] < b["x1"]
                        and b["y0"] < k["y1"] and k["y0"] < b["y1"]) for k in keep):
                keep.append(b)
        out.append({"sheet": f["no"], "frame": f["frame"], "boxes": keep})
    return out


def main():
    d = json.load(open(r"C:\temp\scan.json", encoding="utf-8"))
    res = find_frames(d)
    allb = []
    for r in res:
        print("#%-2d 柜框 %d: %s" % (r["sheet"], len(r["boxes"]),
              " | ".join("%s %dr" % (b["code"] or "?", b["loopnos"]) for b in r["boxes"])))
        allb += r["boxes"]
    json.dump({"boxes": allb, "sheets": res}, open(OUT, "w", encoding="utf-8"),
              ensure_ascii=False, indent=1)
    print("共 %d 台柜 → %s" % (len(allb), OUT))


if __name__ == "__main__":
    main()
