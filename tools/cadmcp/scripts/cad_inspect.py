# cad_inspect.py —— 在 acad.exe 内跑的示例脚本（只读，不改图纸）
# 用法：pyarx_run_file(path="G:/PyArx/tools/cadmcp/scripts/cad_inspect.py")
# 结果同时写 G:/PyArx/log/cad_inspect.json，便于外部读取。

import json
import sys

SUBS = ("Ge", "Enum", "Rx", "Db", "Ed", "Gi", "Core", "Utility", "Debug", "Ap")

# 顶模块可能被 hotbuild 热加载替换成只挂少数子模块的壳，这里按 sys.modules 回填
P = sys.modules.get("PyArxCore")
HOT_SHELL = bool(P is not None and not hasattr(P, "Db"))
if P is None or HOT_SHELL:
    class _Facade(object):
        pass
    f = _Facade()
    for s in SUBS:
        sub = getattr(P, s, None) if P is not None else None
        if sub is None:
            sub = sys.modules.get("PyArxCore." + s)
        if sub is not None:
            setattr(f, s, sub)
    P = f


def main():
    db = P.Db.curDb()
    info = {
        "file": db.getFilename(),
        "entities": len(db.objectIds("AcDbEntity")),
        "texts": len(db.objectIds("AcDbText")),
        "mtexts": len(db.objectIds("AcDbMText")),
        "layers": len(db.objectIds("AcDbLayerTableRecord")),
        "hot_shell": HOT_SHELL,
    }

    out_path = r"G:\PyArx\log\cad_inspect.json"
    try:
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(info, f, ensure_ascii=False, indent=1)
        info["written"] = out_path
    except Exception as e:
        info["write_error"] = repr(e)

    print("cad_inspect: %s ents=%d layers=%d" % (
        info["file"], info["entities"], info["layers"]))
    return info


info = main()
result = info
