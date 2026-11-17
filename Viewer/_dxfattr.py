# -*- coding: utf-8 -*-
"""从 DXF 组码读属性块：ATTDEF 定义 + ATTRIB 实例值。
ATTRIB 的 10/20 已是所属块坐标系里的绝对坐标，按 owner 块名归组，
渲染时随块实例的矩阵变换走到世界坐标即可（和块内其它实体同一套变换）。"""
import json

DXF = r"C:\temp\101_ax.dxf"
OUT = r"C:\temp\101_attrib.json"


def pairs(path):
    with open(path, encoding="utf-8", errors="replace") as f:
        pend = None
        for line in f:
            s = line.strip()
            if pend is None:
                if s:
                    pend = s
                continue
            yield pend, line.rstrip("\r\n")
            pend = None


blkmap = {}      # block handle -> block name
ins_owner = {}   # insert handle -> owner block handle
res = {}
state = None


def flush_att(st):
    own = ins_owner.get(st.get("ins", ""))
    bn = blkmap.get(own, "")
    if not bn or not st["val"].strip():
        return
    if st["flag"] & 1:      # 70 的 bit0 = invisible，不可见属性不该出现在图上
        return
    res.setdefault(bn, {}).setdefault(st.get("ins", ""), []).append(
        [round(st["x"], 2), round(st["y"], 2), st["val"],
         round(st["hh"], 2), st["rot"], st["ha"], st["va"],
         round(st["x2"], 2), round(st["y2"], 2), st["flag"], st["layer"]])


for c, v in pairs(DXF):
    if c == "0":
        ty = v.strip()
        if state is not None:
            if state["t"] in ("BLOCK", "BLOCK_RECORD") and state.get("name"):
                blkmap[state["h"]] = state["name"]
            elif state["t"] == "INSERT":
                ins_owner[state["h"]] = state["owner"]
            elif state["t"] == "ATTRIB":
                flush_att(state)
        if ty in ("BLOCK", "BLOCK_RECORD", "INSERT", "ATTRIB", "SEQEND"):
            state = {"t": ty, "h": "", "owner": "", "name": "", "tag": "", "val": "",
                     "x": 0.0, "y": 0.0, "x2": 0.0, "y2": 0.0,
                     "hh": 1.0, "rot": 0.0, "ha": 0, "va": 0, "flag": 0,
                     "layer": "", "ins": ""}
        else:
            state = {"t": ty}
        continue
    if state is None:
        continue
    t = state["t"]
    if t in ("BLOCK", "BLOCK_RECORD"):
        if c == "5":
            state["h"] = v.strip()
        elif c == "2":
            state["name"] = v.strip()
    elif t == "INSERT":
        if c == "5":
            state["h"] = v.strip()
        elif c == "330":
            state["owner"] = v.strip()
        elif c == "2":
            state["name"] = v.strip()
    elif t == "ATTRIB":
        if c == "330":
            state["ins"] = v.strip()
        elif c == "2":
            state["tag"] = v.strip()
        elif c == "1":
            state["val"] = v.rstrip()
        elif c == "10":
            state["x"] = float(v or 0)
        elif c == "20":
            state["y"] = float(v or 0)
        elif c == "11":
            state["x2"] = float(v or 0)
        elif c == "21":
            state["y2"] = float(v or 0)
        elif c == "72":
            state["ha"] = int(float(v or 0))
        elif c == "74":
            state["va"] = int(float(v or 0))
        elif c == "40":
            state["hh"] = float(v or 0)
        elif c == "50":
            state["rot"] = float(v or 0)
        elif c == "70":
            state["flag"] = int(float(v or 0))
        elif c == "8":
            state["layer"] = v.strip()

json.dump(res, open(OUT, "w", encoding="utf-8"), ensure_ascii=False)

tot = sum(len(x) for v in res.values() for x in v.values())
print("blocks", len(res), "attribs", tot)
for bn, items in sorted(res.items(), key=lambda kv: -sum(len(x) for x in kv[1].values()))[:8]:
    n = sum(len(x) for x in items.values())
    print("  %-46s %5d ins=%d" % (bn[-46:], n, len(items)))
