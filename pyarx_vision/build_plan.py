# -*- utf-8 -*-
"""把 DRB018 系统图统计成 PowerPlan 模型（loadDwgLayout 可直接吃）。

模型分层：
    root  101#中试车间 10kV 总进线
      └ 干线层   图（一）#3 的 25 条干线（101-1-SUB-P1~P25，含容量/整定/电缆）
          └ 箱层   #4~#19 各图的配电箱（31 个）
              └ 回路层  箱内回路（610 条，含断路器/电缆/功率/备用/省略）

省略口径（图纸明确声明）：
    - 图形三点省略 → 该箱 omitted 计数；
    - "WC4~WC10同上" → 区间条数计入 expand_count；
    - "注：图中未见回路为备用" → 未画回路全部为备用。
"""

import io
import json
import os
import re
import sys

sys.path.insert(0, r"G:\PyArx")
from pyarx_vision import frames as F
from pyarx_vision import tablemode as T
from pyarx_vision import sysmode as S

SCAN = r"C:\temp\scan.json"
OUT_STATS = r"G:\PyArx\docs\drb018_system_stats.json"
PLAN = r"G:\PyArx\docs\drb018_powerplan.json"
OUT_PLAN_FULL = r"G:\PyArx\docs\drb018_powerplan_full.json"
OUT_CSV = r"G:\PyArx\docs\drb018_box_loops.csv"
OUT_PLAN_MAIN = r"G:\PyArx\docs\drb018_powerplan_trunk.json"

RE_KW = re.compile(r'([\d.]+)')
RE_NUM = re.compile(r'(\d{1,3})')
RE_CODEREF = re.compile(r'\d{3}-\d{1,2}-\d{1,2}[A-Za-z0-9\-]*[:_]?[A-Za-z0-9]*')
RE_MODELREF = re.compile(r'\b[A-Z]{2,5}-\d{1,2}\b')
ROOT = "101#中试车间总进线"


def clean_desc(s):
    """回路的用途描述：去掉箱编号、型号这类引用，只留用途与参数。"""
    s = RE_CODEREF.sub(" ", s or "")
    s = RE_MODELREF.sub(" ", s)
    return re.sub(r'\s+', ' ', s).strip()


def num(s):
    m = RE_KW.search(s or "")
    return float(m.group(1)) if m else 0.0


def load(scan):
    d = json.load(io.open(scan, encoding="utf-8"))
    fr = F.find_frames(d)
    panels, table = {}, None
    bf = json.load(open(r"G:\PyArx\docs\drb018_boxframes.json", encoding="utf-8"))
    _fb = {}
    for _b in bf["boxes"]:
        _fb.setdefault(_b["sheet"], []).append(_b)
    for f in fr:
        k = F.classify(d, f["frame"])[0]
        if k == "table":
            table = T.parse(d, f["frame"])
        elif k == "panel":
            panels[f["no"]] = S.parse_system(d, f["frame"], _fb.get(f["no"]))
    return d, fr, table, panels


def build(table, panels):
    nodes = [{"code": ROOT, "name": "10kV 总进线", "upstream": "",
              "cable": "", "loadKw": 0.0, "voltage": "10kV", "kind": "root"}]

    # 干线层
    trunk = []
    for it in (table.get("items") or []):
        code = it.get("干线编号", "").strip()
        if not code:
            continue
        node = {"code": code,
                "name": it.get("箱柜编号", "").strip(),
                "upstream": ROOT,
                "cable": (it.get("电缆", "") or "").replace("\n", " ")[:28],
                "loadKw": num(it.get("容量")),
                "voltage": "380",
                "rated": it.get("整定电流", "").strip(),
                "no": it.get("no"),
                "kind": "trunk"}
        nodes.append(node)
        trunk.append(node)

    # 箱层 + 回路层
    orphans = []
    used_codes = set(n["code"] for n in nodes)
    for no, res in sorted(panels.items()):
        for bi, b in enumerate(res["boxes"]):
            code = (b["code"] or "").strip()
            if not code:
                code = "P%d-BOX%d" % (no, bi + 1)
            # 有的是"箱:回路"写法（101-1-1ZAL1:W2），冒号要换成 -W，
            # 否则会和回路层 `箱:回路` 的命名冲突
            code = code.replace(":", "-W")
            # 归属：箱编号里的数字对干线号（101-1-1AL2 → ...-P2）
            up = ""
            m = RE_NUM.findall(code)
            if m:
                cand = "P" + str(int(m[-1]))
                for t in trunk:
                    if t["code"].endswith(cand):
                        up = t["code"]
                        break
            if not up:
                orphans.append(code)
                up = ROOT
            nodes.append({"code": code,
                          "name": b["name"] or b["model"] or "",
                          "upstream": up,
                          "cable": "",
                          "loadKw": b["kw_sum"],
                          "voltage": "380",
                          "sheet": no,
                          "loops": b["drawn"],
                          "omitted": b["omitted"],
                          "expand_count": sum(
                              max(0, int(re.sub(r'[^0-9]', '', s["to"]) or 0)
                                  - int(re.sub(r'[^0-9]', '', s["from"]) or 0) + 1)
                              for s in b["same_expand"]),
                          "note": b["note"],
                          "kind": "box"})
            for l in b["loops"]:
                base = code + ":" + l["no"]
                c = base
                k = 2
                while c in used_codes:      # 同箱同名回路（图上确有分段）
                    c = "%s#%d" % (base, k)
                    k += 1
                used_codes.add(c)
                nodes.append({"code": c,
                              "name": (l["no"] + " " + (clean_desc(l["desc"]) or "备用")).strip(),
                              "upstream": code,
                              "cable": l["cable"][:24],
                              "loadKw": l["kw"],
                              "voltage": "220" if ("照明" in (l["desc"] or "")
                                                   or l["no"].startswith("WL")) else "380",
                              "breaker": l["breaker"][:18],
                              "spare": 1 if l["spare"] else 0,
                              "sheet": no,
                              "kind": "loop"})

    return nodes, trunk, orphans


def main():
    d, fr, table, panels = load(SCAN)
    nodes, trunk, orphans = build(table, panels)

    boxes = [n for n in nodes if n.get("loops") is not None and ":" not in n["code"]]
    loops = [n for n in nodes if ":" in n["code"] and "loops" not in n]
    stats = {
        "source": "101#中试车间(扩建)-设施配电平面及系统_t3.dwg",
        "sheets": {"total": len(fr), "table": 1, "panel": len(panels)},
        "trunk": {"count": len(trunk),
                  "kw": round(sum(t["loadKw"] for t in trunk), 1),
                  "rated_min": min((t["rated"] for t in trunk), default=""),
                  "rated_max": max((t["rated"] for t in trunk), default="")},
        "boxes": {"count": len(boxes),
                  "drawn_loops": sum(b.get("loops", 0) for b in boxes),
                  "omitted_groups": sum(b.get("omitted", 0) for b in boxes),
                  "expand_loops": sum(b.get("expand_count", 0) for b in boxes),
                  "kw": round(sum(b.get("loadKw", 0.0) for b in boxes), 1)},
        "loops": {"count": len(loops),
                  "spare": sum(1 for l in loops if l.get("spare")),
                  "kw": round(sum(l.get("loadKw", 0.0) for l in loops), 1),
                  "with_cable": sum(1 for l in loops if l.get("cable")),
                  "with_breaker": sum(1 for l in loops if l.get("breaker"))},
        "orphans": orphans,
        "note": "省略口径：三点省略=omitted_groups；'X~Y同上'=expand_loops；"
                "'注：图中未见回路为备用'=未画回路全为备用",
    }

    io.open(OUT_STATS, "w", encoding="utf-8").write(
        json.dumps(stats, ensure_ascii=False, indent=2))

    plan = {"dir": "v", "nodes": [
        {k: v for k, v in n.items()
         if k in ("code", "name", "upstream", "cable", "loadKw", "voltage")}
        for n in nodes]}
    io.open(PLAN, "w", encoding="utf-8").write(
        json.dumps(plan, ensure_ascii=False, indent=1))
    # 完整版（带 kind/sheet/omitted 等标记），供逐箱拆分用；
    # 标准版给 loadDwgLayout，多带字段它不认。
    io.open(OUT_PLAN_FULL, "w", encoding="utf-8").write(
        json.dumps({"dir": "v", "nodes": nodes}, ensure_ascii=False, indent=1))

    # 干线级视图（看图用，避免 667 节点挤成一团）
    main_nodes = [n for n in nodes if n["code"] != ROOT and n["upstream"] == ROOT]
    main_nodes = [{"code": ROOT, "name": "10kV 总进线", "upstream": "",
                   "cable": "", "loadKw": 0.0, "voltage": "10kV"}] + main_nodes
    io.open(OUT_PLAN_MAIN, "w", encoding="utf-8").write(
        json.dumps({"dir": "h", "nodes": main_nodes}, ensure_ascii=False, indent=1))

    io.open(os.path.join(OUT_CSV), "w", encoding="utf-8-sig").write(
        "箱编号,回路号,断路器,电缆,功率kW,描述,电压,图纸,备用\n" +
        "\n".join(
            "%s,%s,%s,%s,%s,%s,%s,#%s,%s" % (
                l["upstream"], l["code"].split(":")[-1], l.get("breaker", ""),
                l.get("cable", ""), l.get("loadKw", 0),
                clean_desc(l.get("name", "")), l.get("voltage", ""),
                l.get("sheet", ""), "是" if l.get("spare") else "")
            for l in loops) + "\n")

    print("节点总数", len(nodes), "| 干线", len(trunk), "| 箱", len(boxes),
          "| 回路", len(loops), "| 未归属", len(orphans))
    print("统计", json.dumps(stats, ensure_ascii=False)[:400])


if __name__ == "__main__":
    main()
