"""在 domain_dump.json 上按功能域检查关键操作是否存在。"""
import io
import json

D = json.load(open(r"G:\PyArx\log\domain_dump.json"))

CHECKS = {
    "图元": [
        ("Db", "Line", ["startPoint", "endPoint", "setStartPoint", "setEndPoint"]),
        ("Db", "Circle", ["center", "radius", "setCenter", "setRadius"]),
        ("Db", "Arc", ["startAngle", "endAngle", "setStartAngle", "setEndAngle"]),
        ("Db", "Text", ["textString", "setTextString", "position", "setPosition", "height"]),
        ("Db", "MText", ["contents", "setContents", "location", "setLocation"]),
        ("Db", "Polyline", ["numVerts", "getPoint2dAt", "addVertexAt", "setClosed"]),
        ("Db", "Hatch", ["appendLoop", "setPattern", "patternName"]),
        ("Db", "Spline", ["numControlPoints", "getControlPointAt", "setFitPointCount"]),
        ("Db", "BlockReference", ["blockTableRecord", "position", "setPosition", "attlist"]),
        ("Db", "BlockTableRecord", ["appendAcDbEntity", "append", "objectIds", "getBlockReferences"]),
        ("Db", "Entity", ["erase", "entityType", "getGeomExtents", "transformBy", "setLayer"]),
    ],
    "几何": [
        ("Ge", "Point3d", ["distanceTo", "x", "y", "z", "transformBy"]),
        ("Ge", "Vector3d", ["length", "normalize", "angleTo", "crossProduct", "dotProduct"]),
        ("Ge", "Matrix3d", ["setToRotation", "setToTranslation", "setToScaling", "inverse"]),
        ("Ge", "LineSeg3d", ["length", "startPoint", "endPoint", "midPoint"]),
        ("Ge", "Line3d", ["pointOnLine", "direction"]),
        ("Ge", "Plane", ["normal", "pointOnPlane", "distanceTo"]),
        ("Ge", "Interval", ["length", "lowerBound", "upperBound"]),
        ("Ge", "NurbCurve3d", ["getPointAtParam", "length", "numControlPoints"]),
    ],
    "交互": [
        ("Ed", "Editor", ["getPoint", "getString", "getInt", "getReal", "getKeywords",
                          "setVar", "getVar", "regen", "prompt", "ssget"]),
        ("Ed", "Core", ["getPoint", "getString", "setVar", "getVar", "cmdS", "cmdE"]),
        ("Ed", "Jig", ["drag", "setDispPrompt", "append"]),
        ("Db", "SelectionSet", ["add", "clear", "count"]),
    ],
    "打印": [
        ("Db", "PlotSettings", ["setPlotWindowArea", "plotWindowArea", "setPlotPaperUnits",
                                "setPlotRotation", "setPlotType", "setCanonicalMediaName",
                                "setStyleSheet", "getPlotSettingsValidator"]),
        ("Db", "PlotSettingsValidator", ["setPlotWindowArea", "setPlotPaperSize",
                                         "setPlotRotation", "setPlotType",
                                         "plotSettingsValidatorCurrent"]),
        ("Db", "PlotConfigManager", ["setCurrentConfig", "configs"]),
        ("Db", "PlotEngine", ["beginPlot", "beginDocument", "destroy"]),
        ("Ap", "PlotEngine", ["beginPlot", "beginDocument", "destroy"]),
        ("Db", "PlotStyleTable", ["getPlotStyleTableRecord", "attachToCurrentLayout"]),
        ("Db", "Layout", ["plotSettingsId", "setPlotSettingsId", "getPlotSettings"]),
        ("Ap", "HostApplicationServices", ["getPlotConfigManager", "findConfigFile"]),
    ],
    "读写": [
        ("Db", "Database", ["readDwgFile", "dxfIn", "dxfOut", "saveAs", "writeDwgFile",
                           "createFromDWG", "insert", "wblock"]),
        ("Db", "DwgFiler", ["open", "close", "writeString", "readString"]),
        ("Db", "DxfFiler", ["open", "close", "writeString", "readString"]),
        ("Db", "DatabaseSummaryInfo", ["getTitle", "setTitle"]),
    ],
    "保存": [
        ("Db", "Database", ["saveAs", "save", "isSavedAs", "getFilename", "setFilename",
                           "setDwgVersion", "setDwgFileWasSaved"]),
        ("Db", "DbObject", ["dwgOut", "dwgIn", "dxfOut", "dwgDump"]),
    ],
    "克隆": [
        ("Db", "DbObject", ["cloneFrom", "deepClone", "wblockClone", "clone"]),
        ("Db", "Entity", ["clone", "cloneFrom"]),
        ("Db", "Database", ["deepCloneObjects", "wblockCloneObjects", "abortDeepClone"]),
        ("Db", "IdPair", ["first", "second"]),
    ],
}


def have(mod, cls, attr):
    rec = D.get(mod, {})
    if not isinstance(rec, dict) or "__module__" in rec:
        return False
    members = rec.get(cls)
    if members is None or members == "MISSING" or isinstance(members, str):
        return False
    return attr in members


lines = []
summary = {}
for dom, specs in CHECKS.items():
    lines.append("\n## %s" % dom)
    ok = tot = 0
    for mod, cls, attrs in specs:
        rec = D.get(mod, {})
        cls_missing = (isinstance(rec, dict) and rec.get(cls) in (None, "MISSING")) or ("__module__" in rec)
        if cls_missing:
            lines.append("  %-4s %-24s CLASS MISSING (%d ops)" % (mod, cls, len(attrs)))
            tot += len(attrs)
            continue
        have_n = [a for a in attrs if a in rec[cls]]
        miss_n = [a for a in attrs if a not in rec[cls]]
        ok += len(have_n)
        tot += len(attrs)
        lines.append("  %-4s %-24s %2d/%2d  miss: %s"
                     % (mod, cls, len(have_n), len(attrs), ",".join(miss_n)))
    summary[dom] = (ok, tot)
    lines.append("  --> %s %d/%d" % (dom, ok, tot))

print("\n".join(lines))
print("\n## 汇总")
for dom, (ok, tot) in summary.items():
    print("  %-4s %3d/%3d (%.0f%%)" % (dom, ok, tot, 100.0 * ok / max(tot, 1)))
io.open(r"G:\PyArx\log\domain_audit.txt", "w", encoding="utf-8", newline="").write("\n".join(lines))
