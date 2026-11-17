import json

P = globals().get("P")
if P is None:
    import PyArxCore as P

out = {}


def names(obj):
    return [a for a in dir(obj) if not a.startswith("_")]


db = P.Db.curDb()
out["db_table_methods"] = [a for a in names(db) if ("Table" in a or "layer" in a.lower() or "block" in a.lower())]
out["BlockTable"] = [a for a in names(P.Db.PyDbBlockTable) if not a.startswith("PyDb")]
out["LayerTable"] = [a for a in names(P.Db.PyDbLayerTable) if not a.startswith("PyDb")]
out["SymbolTable"] = [a for a in names(P.Db.PyDbSymbolTable) if not a.startswith("PyDb")]
out["BlockTableRecord"] = [a for a in names(P.Db.PyDbBlockTableRecord) if ("append" in a or "add" in a or a in ("setName", "objectId", "getId", "handle"))]
out["LayerTableRecord_ctor"] = "see error"
out["PyDbObject"] = [a for a in names(P.Db.PyDbObject) if a in ("cast", "as", "objectId", "handle")]

with open(r"G:\PyArx\tools\cadmcp\api_probe_out.json", "w", encoding="utf-8") as f:
    json.dump(out, f, ensure_ascii=False, indent=1)
