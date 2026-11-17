# -*- coding: utf-8 -*-
"""build_capabilities.py —— 把「源码提取的 API 索引」和「无头实跑结果」合成能力矩阵。

输入：
  api_index.jsonl      源码提取的 285 类 / 12156 方法（未验证，可能有签名偏差）
  proto_result.json    proto_verify.py 的无头实跑结果（已验证）
输出：
  api_capabilities.json  结构化矩阵（工具层/模型可读）
  api_capabilities.md    给模型塞 prompt 的紧凑清单

原则：源码里有 ≠ 能用。只有无头实跑通过的才标 verified。
实跑推翻的条目进 known_absent，避免模型再猜。
"""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
IDX = os.path.join(HERE, "api_index.jsonl")
PROTO = os.path.join(HERE, "proto_result.json")
OUT_J = os.path.join(HERE, "api_capabilities.json")
OUT_M = os.path.join(HERE, "api_capabilities.md")

# 本轮无头实跑推翻的「想当然」写法（模型最容易犯的错）
KNOWN_ABSENT = [
    {"name": "PyDbEntity.getBoundingPoints", "why": "61 个方法里不存在；包围盒用 bounds() -> (min,max) 或 getGeomExtents()"},
    {"name": "PyDbEntity.openObjectByIdString", "why": "不存在；按句柄读实物只能事务内 tr.openObject(eid,0) 遍历比对 handle()"},
    {"name": "PyDbEntity.isA", "why": "实体上没有 isA；类名用 className()"},
    {"name": "Ge.AcGePoint3dArray", "why": "Ge 子模块未导出该数组类型；样条改用 setFitPointAt/insertFitPointAt"},
    {"name": "PyDbWipeout.appendLoopPoints", "why": "11 个方法里只有 append；且按 list 传参仍 addEntity=False，遮挡图元构造未打通"},
    {"name": "PyDbMline.appendSegments", "why": "真名是 appendSeg（单词，不是复数）；但只 appendSeg 后 bounds 为空盒"},
    {"name": "PyDbPoint(点)", "why": "无带点构造；PyDbPoint() + setPosition(pt)"},
    {"name": "PyDbSolid(四点)", "why": "无四参构造；PyDbSolid() + setPointAt(i, pt) 四次"},
    {"name": "PyDbXline/Ray(点+方向)", "why": "无带参构造；() + setBasePoint + setUnitDir(AcGeVector3d)"},
    {"name": "PyDbEllipse.setMajorAxis", "why": "不存在；用 setCenter/setMajorRadius/setRadiusRatio"},
    {"name": "PyDbEllipse 半径 setter", "why": "入库前后各试一次，setMajorRadius/setRadiusRatio 均不生效，bounds 恒为默认单位圆 ±1 —— 疑绑定层 setter 缺陷，需查 C++ 侧 PyDbEllipse 的属性桥接"},
]

# 工具层必须遵守的调用约定（实跑得出，非猜测）
PITFALLS = [
    "开对象必须显式给模式：tr.openObject(eid, 0)=kForRead / 1=kForWrite，不许省略",
    "图元/表/符号表默认写模式；纯查询传读模式",
    "符号表在一个进程里只能开一次且必须 close()，否则带写锁存盘会崩",
    "addEntity 返回 bool，失败不抛异常——必须判返回值",
    "新建实体先 addEntity 入模型空间再写属性（入库前未 setDatabaseDefaults）",
    "图层要在事务外建：getLayerTable -> newRecord -> setName/setColor -> lt.add",
]


def load_index():
    cls = {}
    with open(IDX, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            r = json.loads(line)
            c = r.get("c") or ""
            cls.setdefault(c, {"methods": [], "parent": r.get("parent") or ""})
            sig = r.get("sig") or r.get("m")
            if sig:
                cls[c]["methods"].append(sig)
    return cls


def load_proto():
    if not os.path.isfile(PROTO):
        return {}
    with open(PROTO, encoding="utf-8") as f:
        return json.load(f)


def main():
    cls = load_index()
    res = load_proto()
    ents = res.get("entities") or {}
    geo = res.get("geometry") or {}
    sel = res.get("select") or {}

    verified_classes = set()
    for v in ents.values():
        if v.get("ok") and v.get("class"):
            verified_classes.add(v["class"])
    # 几何侧已验证的类（按样板声明）
    for c in ("AcGePoint2d", "AcGePoint3d", "AcGeVector2d", "AcGeVector3d",
              "AcGeMatrix3d", "AcGeLineSeg3d"):
        verified_classes.add(c)

    classes = {}
    for name, info in cls.items():
        classes[name] = {
            "methods": len(info["methods"]),
            "verified": name in verified_classes,
            "parent": info["parent"],
        }

    matrix = {
        "source": {
            "index": os.path.basename(IDX),
            "index_classes": len(cls),
            "index_methods": sum(v["methods"] for v in classes.values()),
            "verified_by": "proto_verify.py (accoreconsole 无头实跑)",
        },
        "verified": {
            "entities": {k: {"class": v.get("class"), "handle": v.get("handle"),
                             "bounds": v.get("bbox")}
                         for k, v in ents.items() if v.get("ok")},
            "entities_failed": {k: v.get("err") for k, v in ents.items() if not v.get("ok")},
            "geometry": {k: {"task": v.get("task"), "got": v.get("got")}
                         for k, v in geo.items() if v.get("ok")},
            "select": sel,
            "summary": res.get("summary") or {},
        },
        "known_absent": KNOWN_ABSENT,
        "pitfalls": PITFALLS,
        "classes": classes,
    }
    with open(OUT_J, "w", encoding="utf-8") as f:
        json.dump(matrix, f, ensure_ascii=False, indent=1)

    # 紧凑清单：模型 prompt 用，只放「能直接用」的部分
    s = matrix["verified"]["summary"]
    lines = [
        "# PyArxCore 能力矩阵（无头实跑验证）",
        "",
        "图元样板 %s/%s 通过、几何运算 %s/%s 通过、选择集读取 %s/%s 通过。"
        % (s.get("ent_ok"), s.get("ent_total"), s.get("geo_ok"),
           s.get("geo_total"), s.get("sel_ok"), s.get("sel_total")),
        "",
        "## 可直接调用的图元构造（参数为图纸单位，角度用度）",
    ]
    def _bad(b):
        """入库成功但几何没有真正生效的判定（否则会误导模型以为可放心用）。"""
        if not isinstance(b, list) or len(b) != 2:
            return True
        try:
            lo, hi = b
            if any(abs(float(x)) > 1e18 for x in list(lo) + list(hi)):
                return True                      # 空盒（1e20）
            return list(lo) == list(hi)          # 退化点盒（point 除外，见调用处）
        except Exception:
            return True

    # 这些图元没有「有限包围盒」这一说，退化盒是正常现象，不能当成未生效
    NO_FINITE_BOX = {"point", "ray", "xline"}
    # 盒形看似有效、但与样板实参不符的：必须单独点名，否则最容易被误用
    MISMATCH = {"ellipse": "半径 setter 未生效（期望 ±40/±20，实际 ±1）"}
    for k, v in sorted((matrix["verified"]["entities"] or {}).items()):
        tag = ""
        if k in MISMATCH:
            tag = "   ← ⚠ " + MISMATCH[k]
        elif k not in NO_FINITE_BOX and _bad(v["bounds"]):
            tag = "   ← ⚠ 已入库但几何未生效，先别用"
        lines.append("- %s -> %s  bounds=%s%s" % (k, v["class"], v["bounds"], tag))
    lines += ["", "## 已验证的几何运算"]
    for k, v in sorted((matrix["verified"]["geometry"] or {}).items()):
        lines.append("- %s：%s -> %s" % (k, v["task"], v["got"]))
    lines += ["", "## 不要使用（实跑已否定）"]
    for d in KNOWN_ABSENT:
        lines.append("- %s —— %s" % (d["name"], d["why"]))
    lines += ["", "## 调用约定"]
    for p in PITFALLS:
        lines.append("- " + p)
    with open(OUT_M, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    print("类 %d / 方法 %d" % (matrix["source"]["index_classes"],
                              matrix["source"]["index_methods"]))
    print("已验证类 %d 个：%s" % (len(verified_classes), ", ".join(sorted(verified_classes))))
    print("写出：%s" % OUT_J)
    print("写出：%s" % OUT_M)


if __name__ == "__main__":
    main()
