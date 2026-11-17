# -*- utf-8 -*-
"""recog.json（识别结果） → trunk_design.json（电力设计画布数据模型）

模型只描述"电气上有什么、怎么连"，不含任何图面坐标风格：
  busbar  母线：id / 编号 / 规格 / 额定 / 站房 / 负荷参数 / 原图几何范围
  breaker 断路器：id / 所属母线 / 整定 / CT / 电缆 / 回路编号 / 计量 / 备用 / 置信度
  edge    连接：bus→breaker（出线），breaker→load（负荷侧，本图未标型号）
自检：id 唯一、每个断路器必须挂到一条母线、回路数与母线声明数守恒。
"""
import json
import os
import re
import collections

R = json.load(open(os.environ.get("RECOG_IN", r"C:\temp\recog.json"), encoding="utf-8"))

problems = []
out = {
    "doc_type": "power-trunk-design",
    "version": "1.0",
    "title": "母线干线系统图 101-1-EL-SYS-21",
    "source": R.get("source", "汇总图-1002.dwg"),
    "units": {"current": "A", "power": "kW", "cable": "mm2"},
    "busbars": [], "breakers": [], "edges": [],
    "stats": {}, "self_check": {},
}

bid = set()
for bi, b in enumerate(R["buses"]):
    n = bi + 1
    bus_id = "BUS-%d" % n
    bid.add(bus_id)
    num = re.search(r"(\d+)#", b["spec"])
    kw = {}
    pe = re.search(r"Pe=(\d+(?:\.\d+)?)", b.get("pe", ""))
    cos = re.search(r"Cos\S?=(\d+(?:\.\d+)?)", b.get("cos", "") or b.get("pe", ""))
    kx = re.search(r"Kx=(\d+(?:\.\d+)?)", b.get("kx", ""))
    ijs = re.search(r"Ijs=(\d+(?:\.\d+)?)", b.get("kx", ""))
    if pe:
        kw["pe_kw"] = float(pe.group(1))
    if cos:
        kw["cos_phi"] = float(cos.group(1))
    if kx:
        kw["kx"] = float(kx.group(1))
    if ijs:
        kw["ijs_a"] = float(ijs.group(1))
    # 规格拆解：相数/型号/防护等级
    spec = {"raw": b["spec"]}
    m = re.search(r"(\d+)A", b["spec"])
    if m:
        spec["rated_a"] = int(m.group(1))
    m = re.search(r"(3P\+N\+PE)", b["spec"])
    if m:
        spec["poles"] = m.group(1)
    m = re.search(r"IP\d+", b["spec"])
    if m:
        spec["ip"] = m.group(0)
    if "铝合金母线槽" in b["spec"]:
        spec["type"] = "铝合金母线槽"
    out["busbars"].append({
        "id": bus_id,
        "no": num.group(1) + "#" if num else str(n),
        "spec": spec,
        "rated_a": spec.get("rated_a", int(re.sub(r"[^0-9]", "", b.get("cap", "0")) or 0)),
        "station": b.get("station", ""),
        "load": kw,
        "geom": {"x0": round(b["x0"], 1), "x1": round(b["x1"], 1), "y": round(b["top"], 1)},
        "loop_count": b["count"],
    })

for bi, b in enumerate(R["buses"]):
    bus_id = "BUS-%d" % (bi + 1)
    for li, lo in enumerate(b["loops"]):
        qf = "QF-%d-%02d" % (bi + 1, li + 1)
        cap = int(re.sub(r"[^0-9]", "", lo.get("cap") or "0") or 0)
        br = {
            "id": qf, "bus": bus_id,
            "rated_a": cap,
            "ct": lo.get("ct", ""),
            "cable": lo.get("cable", ""),
            "circuit_no": lo.get("tag", ""),
            "metering": bool(lo.get("meter")),
            "spare": bool(lo.get("spare")),
            "spare_note": lo.get("spare", ""),
            "source_xy": [lo["x"], lo["y"]],
            "conf": lo.get("conf", {}),
        }
        out["breakers"].append(br)
        out["edges"].append({"from": bus_id, "to": qf, "type": "bus-drop"})
        out["edges"].append({"from": qf, "to": "LOAD-%s" % qf, "type": "feeder"})

# ---- 自检 ----
ids = [x["id"] for x in out["breakers"]]
if len(ids) != len(set(ids)):
    problems.append("断路器 id 重复")
for x in out["breakers"]:
    if x["bus"] not in bid:
        problems.append("%s 挂到了不存在的母线 %s" % (x["id"], x["bus"]))
    if not x["rated_a"]:
        problems.append("%s 缺整定容量" % x["id"])
total_declared = sum(b["loop_count"] for b in out["busbars"])
if total_declared != len(out["breakers"]):
    problems.append("母线声明回路数 %d ≠ 断路器数 %d" % (total_declared, len(out["breakers"])))

spare = sum(1 for x in out["breakers"] if x["spare"])
no_tag = sum(1 for x in out["breakers"] if not x["circuit_no"])
no_cable = sum(1 for x in out["breakers"] if not x["cable"])
out["stats"] = {
    "busbars": len(out["busbars"]),
    "breakers": len(out["breakers"]),
    "edges": len(out["edges"]),
    "spare": spare,
    "rated_total_a": sum(x["rated_a"] for x in out["breakers"]),
    "tags": len(out["breakers"]) - no_tag,
    "no_tag": no_tag,
    "cables": len(out["breakers"]) - no_cable,
    "no_cable": no_cable,
}
out["self_check"] = {"problems": problems, "ok": not problems}

json.dump(out, open(os.environ.get("DESIGN_OUT", r"G:\PyArx\docs\trunk_design.json"), "w", encoding="utf-8"),
          ensure_ascii=False, indent=1)
print("busbars %d  breakers %d  edges %d" %
      (len(out["busbars"]), len(out["breakers"]), len(out["edges"])))
print("备用 %d  有编号 %d  无编号 %d  有电缆 %d  无电缆 %d"
      % (spare, out["stats"]["tags"], no_tag, out["stats"]["cables"], no_cable))
print("自检:", "通过" if not problems else problems)
for b in out["busbars"]:
    print("  %-6s %-5s %-14s %2d 回  %s"
          % (b["no"], b["rated_a"], b["station"], b["loop_count"],
             " ".join("%s=%.4g" % (k, v) for k, v in b["load"].items())))
