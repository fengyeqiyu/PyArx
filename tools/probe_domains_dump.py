"""按功能域 dump 关键类的 API 面：哪些类存在、每个类有哪些成员（只存名字）。"""
import json

import PyArxCore as C

CANDIDATES = {
    "Db": [
        "Database", "DbObject", "Entity", "Curve", "Line", "Circle", "Arc", "Ellipse",
        "Spline", "Polyline", "Polyline2d", "Polyline3d", "Polyline2dVertex", "Polyline3dVertex",
        "Text", "MText", "Hatch", "Region", "Solid", "Trace", "Face", "BlockReference",
        "AttributeReference", "AttributeDefinition", "BlockTable", "BlockTableRecord",
        "LayerTable", "LayerTableRecord", "LinetypeTable", "TextStyleTable", "DimStyleTable",
        "SymbolTable", "SymbolTableRecord", "Dictionary", "Xrecord", "Transaction",
        "Layout", "PlotSettings", "PlotSettingsValidator", "PlotConfigManager", "PlotConfig",
        "PlotEngine", "PlotProgressDialog", "PlotStyleTable", "PlotStyleTableRecord",
        "DwgFiler", "DxfFiler", "DatabaseSummaryInfo", "ObjectId", "Handle", "Extents",
        "Group", "Viewport", "ViewportTable", "ViewportTableRecord", "UcsTable", "RegAppTable",
        "DimStyleTableRecord", "Dimension", "MLeader", "Table", "TableStyle", "GeoData",
        "Surface", "SubDMesh", "SubEntPath", "ObjectIdArray", "FullSubentityPath",
    ],
    "Ge": [
        "Point2d", "Point3d", "Vector2d", "Vector3d", "Matrix2d", "Matrix3d", "Scale3d",
        "Line2d", "Line3d", "LineSeg2d", "LineSeg3d", "CircularArc2d", "CircularArc3d",
        "Curve2d", "Curve3d", "EllipticalArc2d", "NurbCurve2d", "NurbCurve3d",
        "Plane", "BoundedPlane", "Cylinder", "Sphere", "Torus", "Cone", "Interval",
        "Extents3d", "Extents2d", "KnotCollection", "PointOnCurve3d",
    ],
    "Ed": [
        "Editor", "Core", "Jig", "EdJig", "EditorReactor", "InputPointManager",
        "SelectionSet", "PromptStatus", "DragStatus", "UserInputControls", "InputPointMonitor",
    ],
    "Ap": [
        "Application", "DocManager", "Document", "HostApplicationServices", "PlotEngine",
        "LayoutManager", "StatusBar", "PaletteSet", "DocLockMode", "PyRxTestFlags",
    ],
    "Rx": ["RxObject", "RxClass", "ProtocolExt", "RxProtocolReactor"],
}

out = {}
for mod_name, names in CANDIDATES.items():
    mod = getattr(C, mod_name, None)
    if mod is None:
        out[mod_name] = {"__module__": "MISSING"}
        continue
    rec = {}
    for n in names:
        obj = getattr(mod, n, None)
        if obj is None:
            rec[n] = "MISSING"
        else:
            try:
                rec[n] = sorted(x for x in dir(obj) if not x.startswith("__"))
            except Exception as ex:
                rec[n] = "ERR:%s" % type(ex).__name__
    out[mod_name] = rec

with open(r"G:/PyArx/log/domain_dump.json", "w") as f:
    json.dump(out, f)

for mod_name, rec in out.items():
    if isinstance(rec, dict) and "__module__" in rec:
        print("[DOM] %s MODULE MISSING" % mod_name)
        continue
    have = [k for k, v in rec.items() if v != "MISSING"]
    miss = [k for k, v in rec.items() if v == "MISSING"]
    print("[DOM] %s have=%d missing=%d :: %s" % (mod_name, len(have), len(miss), ",".join(miss)))
