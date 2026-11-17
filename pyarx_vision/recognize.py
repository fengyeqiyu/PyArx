# -*- utf-8 -*-
"""pyarx_vision.recognize —— 识别主流程与结果对象

流程：校验 → 矩形归一 → 阵列发现 → 母线发现 → 图种判定
      → 干线图走"母线信息 + 回路归属 + 自检"，其他图只出结构统计
结果统一是 Result，可 to_design() 转成电力设计数据模型。
"""
import re

from . import schema, detect, semantics, geometry as G

DEFAULT_CONFIG = {
    "unit_min_count": 8,      # 阵列单元至少几个才算阵列
    "unit_size_tol": 50.0,    # 同尺寸容差（图纸单位）
    "row_tol": 1500.0,        # 同"行"的 y 容差
    "max_dev": 0.15,          # 等距性允许的相对偏差
    "bus_min_len_ratio": 0.15,  # 母线候选线长 ≥ 图宽 15%
    "bus_y_tol": 1500.0,
    "bus_gap": 1500.0,        # x 空档超过它才拆成两段母线
    "bus_pad": 800.0,
    "bus_dy": 6000.0,
    "row_group_tol": 1200.0,
}


class Result(object):
    """识别结果。problems 非空即视为不可信，必须暴露给使用者。"""

    def __init__(self, kind, buses=None, loops=None, units=None, pools=None,
                 problems=None, log=None, stats=None, frame=None, source=""):
        self.kind = kind
        self.buses = buses or []
        self.loops = loops or []
        self.units = units or []
        self.pools = pools or {}
        self.problems = problems or []
        self.log = log or []
        self.stats = stats or {}
        self.frame = frame
        self.source = source

    @property
    def ok(self):
        return not self.problems

    def summary(self):
        return ("kind=%s buses=%d loops=%d units=%d texts=%d problems=%d"
                % (self.kind, len(self.buses), len(self.loops), len(self.units),
                   sum(len(v) for v in self.pools.values()), len(self.problems)))

    # ------------------------------------------------------------ 转设计模型
    def to_design(self):
        """识别结果 → 电力设计数据模型（母线 / 断路器 / 连接）"""
        out = {"doc_type": "power-trunk-design", "version": "1.0",
               "title": "母线干线系统图", "source": self.source,
               "units": {"current": "A", "power": "kW", "cable": "mm2"},
               "busbars": [], "breakers": [], "edges": [],
               "stats": {}, "self_check": {"problems": list(self.problems),
                                           "ok": self.ok}}
        if self.kind != "trunk":
            out["doc_type"] = "power-plan-structure"
            out["stats"] = self.stats
            return out

        def num(s):
            m = re.search(r"(\d+(?:\.\d+)?)", s or "")
            return float(m.group(1)) if m else 0.0

        for bi, b in enumerate(self.buses):
            bus_id = "BUS-%d" % (bi + 1)
            no = re.search(r"(\d+)#", b.get("spec", ""))
            spec = {"raw": b.get("spec", "")}
            rated = int(num(b.get("cap")) or 0)
            spec["rated_a"] = rated
            m = re.search(r"(3P[+N+PE]*|1P[+N+PE]*)", b.get("spec", ""))
            if m:
                spec["poles"] = m.group(1)
            m = re.search(r"(IP\d+)", b.get("spec", ""))
            if m:
                spec["ip"] = m.group(1)
            if "母线槽" in b.get("spec", ""):
                spec["type"] = "母线槽"
            out["busbars"].append({
                "id": bus_id,
                "no": (no.group(1) + "#") if no else "%d#" % (bi + 1),
                "spec": spec,
                "rated_a": rated,
                "station": b.get("station", ""),
                "load": {"pe_kw": num(b.get("pe")),
                         "cos_phi": num(b.get("cos")),
                         "kx": num(b.get("kx")),
                         "ijs_a": num(re.search(r"Ijs\s*=\s*([^\s]+)", b.get("kx", "")).group(1)
                                      if "Ijs" in b.get("kx", "") else "")},
                "geom": {"x0": round(b["x0"], 1), "x1": round(b["x1"], 1),
                         "y": round(b["top"], 1)},
                "loop_count": b.get("count", 0),
            })
            for li, lo in enumerate(b.get("loops", [])):
                qf = "QF-%d-%02d" % (bi + 1, li + 1)
                out["breakers"].append({
                    "id": qf, "bus": bus_id,
                    "rated_a": int(num(lo.get("cap"))),
                    "ct": lo.get("ct", ""), "cable": lo.get("cable", ""),
                    "circuit_no": lo.get("tag", ""),
                    "metering": bool(lo.get("meter")),
                    "spare": bool(lo.get("spare")),
                    "spare_note": lo.get("spare", ""),
                    "source_xy": [round(lo["x"], 1), round(lo["y"], 1)],
                    "conf": lo.get("conf", {}),
                })
                out["edges"].append({"from": bus_id, "to": qf, "type": "bus-drop"})
                out["edges"].append({"from": qf, "to": "LOAD-%s" % qf, "type": "feeder"})

        out["stats"] = {
            "busbars": len(out["busbars"]), "breakers": len(out["breakers"]),
            "edges": len(out["edges"]),
            "spare": sum(1 for b in out["breakers"] if b["spare"]),
            "no_tag": sum(1 for b in out["breakers"] if not b["circuit_no"]),
            "no_cable": sum(1 for b in out["breakers"] if not b["cable"]),
        }
        # 设计层自检
        ids = [b["id"] for b in out["breakers"]]
        if len(ids) != len(set(ids)):
            out["self_check"]["problems"].append("断路器 id 重复")
        declared = sum(b["loop_count"] for b in out["busbars"])
        if declared != len(out["breakers"]):
            out["self_check"]["problems"].append(
                "母线声明回路数 %d ≠ 断路器数 %d" % (declared, len(out["breakers"])))
        out["self_check"]["ok"] = not out["self_check"]["problems"]
        return out


def recognize(es, config=None):
    cfg = dict(DEFAULT_CONFIG)
    cfg.update(config or {})
    problems = schema.validate(es)
    log = []

    rects = detect.normalize_rects(es)
    units = detect.find_units(rects, min_count=cfg["unit_min_count"],
                              size_tol=cfg["unit_size_tol"],
                              row_tol=cfg["row_tol"], max_dev=cfg["max_dev"])
    frame = es.get("frame") or schema.bbox(es) or (0, 0, 1, 1)
    fw = frame[2] - frame[0] or 1
    buses = detect.find_buses(es, fw, min_len_ratio=cfg["bus_min_len_ratio"],
                              y_tol=cfg["bus_y_tol"], gap=cfg["bus_gap"])
    pools = semantics.classify_all(es)

    log.append("图元 lines=%d plines=%d texts=%d circles=%d"
               % (len(es["lines"]), len(es["plines"]), len(es["texts"]), len(es["circles"])))
    log.append("矩形 %d，阵列类型 %d，母线段 %d" % (len(rects), len(units), len(buses)))
    for u in units[:3]:
        log.append("  阵列 %s×%s：%d 个 / %d 行，列距 %.1f"
                   % (u["size"][0], u["size"][1], u["count"], u["rows_n"], u["step"]))
    log.append("文字分类 " + " ".join("%s=%d" % (k, len(v)) for k, v in sorted(pools.items())))

    # ---- 图种判定：有主阵列 + 至少两条母线 + 有母线/容量语义 → 干线图 ----
    has_sem = bool(pools.get("bus")) or len(pools.get("cap", [])) >= 2
    kind = "trunk" if (units and len(buses) >= 2 and has_sem) else "plan"
    if kind == "plan":
        lens = [abs(l["a"][0] - l["b"][0]) + abs(l["a"][1] - l["b"][1]) for l in es["lines"]]
        stats = {"lines": len(es["lines"]), "texts": len(es["texts"]),
                 "plines": len(es["plines"]),
                 "total_len": round(sum(lens), 1),
                 "max_len": round(max(lens), 1) if lens else 0,
                 "text_classes": {k: len(v) for k, v in pools.items()}}
        log.append("判定为平面/桥架图：只做结构统计，不解析母线回路")
        if not units:
            log.append("  未发现等距阵列单元")
        return Result("plan", units=units, pools=pools, log=log, stats=stats,
                      frame=frame, source=es.get("source", ""))

    # ---- 干线图 ----
    semantics.bus_info(buses, pools)
    main = units[0]
    loops = []
    for it in main["items"]:
        cx, cy = G.rect_center(it["r"])
        bi = detect.assign_bus((cx, cy), buses, pad=cfg["bus_pad"], max_dy=cfg["bus_dy"])
        loops.append({"x": cx, "y": cy, "bus": bi, "cap": "", "tag": "",
                      "cable": "", "ct": "", "meter": False, "spare": ""})
    stats_assign = semantics.assign(loops, pools, size_key="cap")
    for k, (hit, cand) in stats_assign.items():
        log.append("  归属 %-6s 命中 %d/%d" % (k, hit, cand))

    for bi, b in enumerate(buses):
        b["loops"] = sorted([l for l in loops if l["bus"] == bi], key=lambda r: r["x"])
        b["count"] = len(b["loops"])

    # ---- 自检 ----
    if len(loops) != main["count"]:
        problems.append("回路数 %d 与阵列单元数 %d 不一致" % (len(loops), main["count"]))
    bad = sum(1 for l in loops if l["bus"] < 0)
    if bad:
        problems.append("%d 个回路未归属母线" % bad)
    ncap = sum(1 for l in loops if not l["cap"])
    if ncap:
        problems.append("%d 个回路缺整定容量" % ncap)
    for bi, b in enumerate(buses):
        if not b["count"]:
            problems.append("母线段 %d 无回路" % (bi + 1))
            continue
        gaps = [b["loops"][i + 1]["x"] - b["loops"][i]["x"] for i in range(b["count"] - 1)]
        if gaps and (max(gaps) - min(gaps)) / max(gaps) > 0.25:
            problems.append("母线段 %d 列距不均匀 %.0f..%.0f" % (bi + 1, min(gaps), max(gaps)))
    total = sum(b["count"] for b in buses)
    log.append("母线段回路数 %s = %d" % ("+".join(str(b["count"]) for b in buses), total))

    stats = {"busbars": len(buses), "loops": len(loops),
             "units": main["count"], "step": round(main["step"], 1),
             "spare": sum(1 for l in loops if l["spare"]),
             "no_tag": sum(1 for l in loops if not l["tag"]),
             "no_cable": sum(1 for l in loops if not l["cable"]),
             "text_classes": {k: len(v) for k, v in pools.items()}}
    return Result("trunk", buses=buses, loops=loops, units=units, pools=pools,
                  problems=problems, log=log, stats=stats, frame=frame,
                  source=es.get("source", ""))
