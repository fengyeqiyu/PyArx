import json

P = globals().get("P")
if P is None:
    import PyArxCore as P

log = {}
OUT = r"G:\PyArx\tools\cadmcp\api_probe5_out.json"


def rec(k, fn):
    try:
        log[k] = {"ok": True, "val": str(fn())[:400]}
    except Exception as e:
        log[k] = {"ok": False, "err": type(e).__name__ + ": " + str(e)[:500]}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(log, f, ensure_ascii=False, indent=1)


db = P.Db.curDb()

rec("TvList_methods", lambda: [a for a in dir(P.Db.TvList) if not a.startswith("_")])
rec("TvList_ctor", lambda: type(P.Db.TvList()).__name__)


def layer_by_menucmd():
    r = P.Ed.acedMenucmd('(command "_.-LAYER" "_N" "PYX_L2" "")')
    return "ret=" + str(r)


rec("layer_by_menucmd", layer_by_menucmd)


def layer_exists():
    with P.Db.PyDbTrans() as tr:
        lt = db.getLayerTable()
        return {"has": bool(lt.has("PYX_L2")), "names": list(lt.names())[:8]}


rec("layer_exists", layer_exists)

log["done"] = True
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=1)
