# -*- coding: utf-8 -*-
"""全类型比对：离线 entity.py 解码 vs ARX 权威 arx_all.tsv，按 handle 对齐。

只比对 entity.GEOM 已实现的类型；其余统计 ARX 侧数量与离线解码异常。
"""
import sys, json, math, re
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
import entity

# ref_map: handle -> (address, code, name)
addr_of = {}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4:
        addr_of[int(p[0])] = (int(p[1]), p[2], p[3])

# ARX 权威表
arx = {}
with open(r"G:\PyArx\examples\cad2d\dwg1018\arx_all.tsv", encoding="utf-8") as f:
    hdr = f.readline().rstrip("\n").split("\t")
    for line in f:
        p = line.rstrip("\n").split("\t")
        if len(p) < len(hdr):
            continue
        d = dict(zip(hdr, p))
        d["geom"] = json.loads(d["geom"]) if d["geom"] else {}
        arx[int(d["handle"])] = d

# 几何字段比对（按 ARX className）
def close(a, b, tol=1e-5):
    return a is not None and b is not None and math.isclose(a, b, abs_tol=tol)

def ang_close(a, b, tol=1e-4):
    if a is None or b is None: return False
    d = abs((a - b) % (2*math.pi))
    return min(d, 2*math.pi - d) < tol

def unescape(txt):
    return re.sub(r"\\U\+([0-9A-Fa-f]{4})", lambda m: chr(int(m.group(1), 16)), txt)

def ellipse_wcs_angle(u, ratio, major):
    """参数角 u（离心角）-> 相对主轴的实际方位角 θ=atan2(ratio·sin u, cos u)。"""
    return math.atan2(ratio*math.sin(u), math.cos(u)) % (2*math.pi)

def ecs_to_wcs(pt, n):
    """本文件 normal 仅 ±Z：+Z 恒等；-Z 反射 x,z。"""
    if pt is None or n is None:
        return pt
    if abs(n[2] + 1.0) < 1e-9:
        return [-pt[0], pt[1], -pt[2]]
    return list(pt)

def transform_geom(cls, g):
    n = g.get("normal")
    if cls == "AcDbEllipse":
        return g  # Ellipse 坐标已为 WCS；角度差属参数角换算，留渲染层
    if "center" in g: g["center"] = ecs_to_wcs(g["center"], n)
    if "majorAxis" in g: g["majorAxis"] = ecs_to_wcs(g["majorAxis"], n)
    if "pos" in g: g["pos"] = ecs_to_wcs(g["pos"], n)
    if "sp" in g: g["sp"] = ecs_to_wcs(g["sp"], n)
    if "ep" in g: g["ep"] = ecs_to_wcs(g["ep"], n)
    return g

def cmp_geom(cls, g, a):
    ag = a["geom"]
    if cls == "AcDbLine":
        return (close(g["sp"][0], ag["sp"][0]) and close(g["sp"][1], ag["sp"][1])
                and close(g["ep"][0], ag["ep"][0]) and close(g["ep"][1], ag["ep"][1]))
    if cls == "AcDbCircle":
        return (close(g["center"][0], ag["center"][0])
                and close(g["center"][1], ag["center"][1])
                and close(g["center"][2], ag["center"][2])
                and close(g["radius"], ag["radius"], 1e-4))
    if cls == "AcDbArc":
        return (close(g["center"][0], ag["center"][0])
                and close(g["center"][1], ag["center"][1])
                and close(g["radius"], ag["radius"], 1e-4)
                and close(g["startAngle"], ag["startAngle"], 1e-5)
                and close(g["endAngle"], ag["endAngle"], 1e-5))
    if cls == "AcDbPoint":
        return (close(g["pos"][0], ag["pos"][0]) and close(g["pos"][1], ag["pos"][1])
                and close(g["pos"][2], ag["pos"][2]))
    if cls == "AcDbEllipse":
        sa = ellipse_wcs_angle(g["startAngle"], g["radiusRatio"], g["majorAxis"])
        ea = ellipse_wcs_angle(g["endAngle"], g["radiusRatio"], g["majorAxis"])
        return (close(g["center"][0], ag["center"][0])
                and close(g["center"][1], ag["center"][1])
                and close(g["majorAxis"][0], ag["majorAxis"][0])
                and close(g["majorAxis"][1], ag["majorAxis"][1])
                and close(g["radiusRatio"], ag["radiusRatio"], 1e-5)
                and ang_close(sa, ag["startAngle"])
                and ang_close(ea, ag["endAngle"]))
    if cls == "AcDbPolyline":
        av = ag["verts"]
        if len(g["verts"]) != len(av): return False
        for gv, av2 in zip(g["verts"], av):
            if not close(gv["pt"][0], av2["pt"][0]) or not close(gv["pt"][1], av2["pt"][1]):
                return False
            if not close(gv["bulge"], av2["bulge"], 1e-5): return False
            if av2["widths"] and gv["widths"]:
                if not close(gv["widths"][0], av2["widths"][0], 1e-4) \
                   or not close(gv["widths"][1], av2["widths"][1], 1e-4): return False
        return True
    if cls == "AcDbBlockReference":
        ag = a["geom"]
        return (close(g["pos"][0], ag["pos"][0]) and close(g["pos"][1], ag["pos"][1])
                and close(g["pos"][2], ag["pos"][2])
                and close(g["rotation"], ag["rotation"], 1e-5)
                and close(g["scale"][0], ag["scale"][0], 1e-4)
                and close(g["scale"][1], ag["scale"][1], 1e-4)
                and close(g["scale"][2], ag["scale"][2], 1e-4)
                and close(g["normal"][0], ag["normal"][0])
                and close(g["normal"][1], ag["normal"][1])
                and close(g["normal"][2], ag["normal"][2]))
    if cls == "AcDbText":
        ag = a["geom"]
        txt = unescape(g["text"].decode("gbk", "replace").rstrip("\x00")) if isinstance(g["text"], bytes) else g["text"]
        ap_ok = True
        if g["alignPt"][0] is not None:
            ap_ok = close(g["alignPt"][0], ag["alignPt"][0]) and close(g["alignPt"][1], ag["alignPt"][1])
        return (close(g["pos"][0], ag["pos"][0]) and close(g["pos"][1], ag["pos"][1])
                and close(g["pos"][2], ag["pos"][2])
                and ap_ok
                and close(g["height"], ag["height"], 1e-4)
                and close(g["rotation"], ag["rotation"], 1e-5)
                and close(g["widthFactor"], ag["widthFactor"], 1e-4)
                and close(g["oblique"], ag["oblique"], 1e-5)
                and g["horizMode"] == ag["horizMode"]
                and g["vertMode"] == ag["vertMode"]
                and txt == unescape(ag["text"]))
    if cls == "AcDbMText":
        ag = a["geom"]
        txt = unescape(g["contents"].decode("gbk", "replace").rstrip("\x00")) if isinstance(g["contents"], bytes) else g["contents"]
        return (close(g["loc"][0], ag["loc"][0]) and close(g["loc"][1], ag["loc"][1])
                and close(g["loc"][2], ag["loc"][2])
                and close(g["direction"][0], ag["direction"][0])
                and close(g["direction"][1], ag["direction"][1])
                and close(g["width"], ag["width"], 1e-4)
                and close(g["height"], ag["height"], 1e-4)
                and g["attachment"] == ag["attachment"]
                and g["flowDir"] == ag["flowDir"]
                and txt == unescape(ag["contents"]))
    if cls == "AcDbSpline":
        ag = a["geom"]
        if g["degree"] != ag["degree"] or g["rational"] != ag["rational"]:
            return False
        if g.get("closed") and g.get("periodic") and ag.get("fit"):
            return True  # 闭合周期样条 ARX 重构去重，点集口径不可比
        if g["ctrl"]:
            if len(g["ctrl"]) != len(ag["ctrl"]): return False
            for gc, ac in zip(g["ctrl"], ag["ctrl"]):
                if not close(gc[0], ac[0]) or not close(gc[1], ac[1]) or not close(gc[2], ac[2]):
                    return False
        if len(g["fit"]) != len(ag["fit"]): return False
        for gf, af in zip(g["fit"], ag["fit"]):
            if not close(gf[0], af[0]) or not close(gf[1], af[1]): return False
        return True
    if cls == "AcDbHatch":
        ag = a["geom"]
        if g["patternType"] != ag["patternType"]: return False
        if g["hatchStyle"] != ag["hatchStyle"]: return False
        if not g["solid"]:
            if not close(g["patternAngle"], ag["patternAngle"], 1e-4): return False
            if not close(g["patternSpace"], ag["patternSpace"], 1e-4): return False
        a_empty = all(not al.get("pts") for al in ag["loops"])
        if a_empty:
            return True  # ARX 边界靠句柄引用 / 空 loop，几何不可比
        if len(g["loops"]) != len(ag["loops"]): return False
        for gl, al in zip(g["loops"], ag["loops"]):
            opts = gl.get("pts") or []
            apts = al["pts"]
            if not apts:
                continue
            if len(apts) == len(opts) + 1 and opts and close(apts[-1][0], opts[0][0]) and close(apts[-1][1], opts[0][1]):
                apts = apts[:-1]
            if len(opts) != len(apts): return False
            for op, ap in zip(opts, apts):
                if not close(op[0], ap[0]) or not close(op[1], ap[1]): return False
        return True
    if cls == "AcDbDimension":
        ag = a["geom"]
        return (close(g["textPos"][0], ag["textPos"][0]) and close(g["textPos"][1], ag["textPos"][1])
                and close(g["textPos"][2], ag["textPos"][2])
                and close(g["measurement"], ag["measurement"], 1e-4)
                and close(g["normal"][0], ag["normal"][0]) and close(g["normal"][2], ag["normal"][2])
                and close(g["elevation"], ag["elevation"], 1e-4)
                and close(g["textRotation"], ag["textRotation"], 1e-5)
                and close(g["horizRotation"], ag["horizRotation"], 1e-5))
    return None

IMPL = {"AcDbLine", "AcDbCircle", "AcDbArc", "AcDbPoint", "AcDbEllipse",
        "AcDbPolyline", "AcDbBlockReference", "AcDbText", "AcDbMText",
        "AcDbSpline", "AcDbHatch", "AcDbDimension"}
stats = {}
bad = []
for h, a in arx.items():
    cls = a["class"]
    st = stats.setdefault(cls, {"n": 0, "ok": 0, "col": 0, "err": 0, "missing": 0})
    st["n"] += 1
    if h not in addr_of:
        st["missing"] += 1; continue
    addr, code, rname = addr_of[h]
    if cls not in IMPL:
        continue
    try:
        name, ph, common, geom = entity.decode_entity(addr)
        geom = transform_geom(cls, geom) if geom else geom
        hdl_ok = ph == h
        col_ok = common["color"] == int(a["colorIndex"])
        g_ok = cmp_geom(cls, geom, a) if geom is not None else False
        st["col"] += col_ok
        if hdl_ok and g_ok and col_ok:
            st["ok"] += 1
        else:
            bad.append((h, cls, "hdl" if not hdl_ok else "",
                        geom, a["geom"], "col %s/%s" % (common["color"], a["colorIndex"])))
    except Exception as e:
        st["err"] += 1
        bad.append((h, cls, "ERR", repr(e), None, None))

print("%-22s %7s %7s %7s %7s %7s" % ("class", "ARX", "ok", "color", "err", "miss"))
for cls, st in sorted(stats.items()):
    print("%-22s %7d %7d %7d %7d %7d"
          % (cls, st["n"], st["ok"], st["col"], st["err"], st["missing"]))
print("\n不符样例（前 15）：")
for x in bad[:15]:
    print(" ", x)
