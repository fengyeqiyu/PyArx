# -*- coding: utf-8 -*-
"""合并三个实测 shard（+旧矩阵补录 PyDbCircle）为 module_matrix.json。

- 按 (layer, module) 去重，断言数多的一份为准
- extra_scan 不是模块：不计入模块数，但其清单事实在蒸馏阶段使用
- summary 重算 pass/fail
- 四类断言覆盖：存在与构造 / 属性往返 / 闭环 / 失败面
"""
import json
import os

BASE = r"G:\PyArx\tools\train\data"
SOURCES = ["shard_db.json", "shard_ge_gi.json", "shard_ed_app_engine.json", "module_matrix.json"]
# 旧矩阵里刻意回填、不在三 shard 中的模块（PyDbCircle 旧实测记录）
BACKFILL = {("Db", "PyDbCircle")}

CATS = ["exists_construct", "prop_roundtrip", "closed_loop", "failure_face"]


def classify(name, detail):
    n = name.lower()
    d = detail.lower()
    hit = set()
    if any(k in n for k in ("exists", "ctor", "name_trap", "trap_missing", "missing",
                             "static_property", "count_curdoc", "absent_scan", "opaque",
                             "arxlist", "registered_executed", "type_name", "members",
                             "enum_values", "data", "bisect", "headless_interactive",
                             "unsupported_members", "exists_missing")):
        hit.add("exists_construct")
    if any(k in n for k in ("prop_roundtrip", "trait_roundtrip", "class_constant",
                            "distanceto", "length", "lengthsqrd", "length_", "dot", "cross",
                            "normalize", "negate", "asvector", "coefficients", "signeddistance",
                            "start_end", "degenerate", "getmatrix", "extractscale", "trait",
                            "readback", "x_all", "make_line", "returns_last", "handle_to",
                            "mode_x", "name_title", "curdb", "new_db", "open_current",
                            "open_existing", "dbtext", "xscale", "roundtrip", "curdoc",
                            "count", "filevalid", "bool_trait", "read_only", "evalpoint",
                            "midpoint", "normal", "osmode", "registry", "bisect", "data")):
        hit.add("prop_roundtrip")
    if any(k in n for k in ("db_roundtrip", "readback_after_commit", "openobject_readback",
                             "closed_loop", "loop_inverse", "ctx_add", "dbtext_closed",
                             "loop", "make_line+readback", "open_object")):
        hit.add("closed_loop")
    if any(k in n for k in ("[probe]", "headless", "negative", "silent", "crash", "trap",
                            "absent", "unavailable", "degrade", "unsupported", "unknown",
                            "missing", "degenerate")):
        hit.add("failure_face")
    return hit


def main():
    merged = {}
    scans = []
    for fn in SOURCES:
        p = os.path.join(BASE, fn)
        if not os.path.isfile(p):
            continue
        with open(p, encoding="utf-8") as f:
            data = json.load(f)
        for m in data:
            if m["module"] == "extra_scan":
                scans.append({"source": fn, "layer": m["layer"], "checks": m["checks"]})
                continue
            key = (m["layer"], m["module"])
            if fn == "module_matrix.json" and key not in BACKFILL:
                continue
            if key not in merged or len(m["checks"]) > len(merged[key]["checks"]):
                merged[key] = m

    matrix = []
    missing_classes = []
    layer_stat = {}
    for (layer, module), m in sorted(merged.items()):
        checks = m["checks"]
        passed = sum(1 for c in checks if c["ok"])
        failed = sum(1 for c in checks if not c["ok"])
        cats = set()
        for c in checks:
            cats |= classify(c["name"], c["detail"])
        miss = [c for c in CATS if c not in cats]
        if miss:
            missing_classes.append({"layer": layer, "module": module, "missing": miss})
        entry = {"layer": layer, "module": module, "checks": checks,
                  "summary": {"pass": passed, "fail": failed}}
        matrix.append(entry)
        st = layer_stat.setdefault(layer, {"modules": 0, "checks": 0})
        st["modules"] += 1
        st["checks"] += len(checks)

    out = {"matrix": matrix, "scans": scans, "layer_stat": layer_stat,
           "missing_classes": missing_classes}
    with open(os.path.join(BASE, "module_matrix.json"), "w", encoding="utf-8") as f:
        json.dump(matrix, f, ensure_ascii=False, indent=1)

    print("modules=%d checks=%d" % (len(matrix), sum(len(m["checks"]) for m in matrix)))
    for L, st in sorted(layer_stat.items()):
        print("  %-7s modules=%2d checks=%3d" % (L, st["modules"], st["checks"]))
    print("missing_class modules=%d" % len(missing_classes))
    for mc in missing_classes:
        print("  MISS", mc["layer"], mc["module"], mc["missing"])


if __name__ == "__main__":
    main()
