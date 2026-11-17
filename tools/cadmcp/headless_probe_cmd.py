import json

P = globals().get("P")
if P is None:
    import PyArxCore as P

log = {}
OUT = r"G:\PyArx\tools\cadmcp\api_probe4_out.json"


def rec(k, fn):
    try:
        log[k] = {"ok": True, "val": str(fn())[:400]}
    except Exception as e:
        log[k] = {"ok": False, "err": type(e).__name__ + ": " + str(e)[:500]}
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(log, f, ensure_ascii=False, indent=1)


db = P.Db.curDb()
rec("acedCmd_sig", lambda: P.Ed.acedCmd(123))
rec("acedMenucmd_sig", lambda: P.Ed.acedMenucmd(123))


def layer_by_cmd():
    P.Ed.acedCmd(["_.-LAYER", "_N", "PYX_L1", ""])
    return "sent"


rec("layer_by_cmd", layer_by_cmd)


def layer_exists():
    with P.Db.PyDbTrans() as tr:
        lt = db.getLayerTable()
        return {"has": bool(lt.has("PYX_L1")), "names": list(lt.names())[:6]}


rec("layer_exists", layer_exists)

log["done"] = True
with open(OUT, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=1)
