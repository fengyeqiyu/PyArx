"""reload.py —— 在 acad.exe 内热替换 PyArxCore（先清模块，再导入）。

步骤（顺序不可变）：
  1. 读 module.txt 拿到本次 hotbuild 产出的模块名（PyArxCoreH<tag>）；
  2. 【清空】把所有已导入的 PyArxCore / PyArxCore.* 从 sys.modules 里摘掉，
     旧模块对象只挂到 __pyarx_old__ 列表保活（避免旧对象析构撞上新注册表）；
  3. 若新模块名此前残留过（上次导入失败），也一并摘掉，保证本次是干净 import；
  4. import 新模块 → 其入口把 sys.modules["PyArxCore"] 指向自身；
  5. 自检本次必须存在的接口（documents/documentAt/saveAll/quit、Document.save/saveAs）。

用法（CAD 内）：exec(open(r"G:\\PyArx\\tools\\hotbuild\\reload.py").read())
"""

import importlib
import json
import sys
import traceback

OUTDIR = r"G:\PyArx\X64\Release"
MODTXT = r"G:\PyArx\tools\hotbuild\module.txt"
RESULT = r"G:\PyArx\tools\hotbuild\reload_out.json"

# 本次必须自检的接口（缺一个就说明 hot 模块没接管成功）
NEED_DM = ("documents", "documentAt", "saveAll", "quit", "count", "curDocument")
NEED_DOC = ("save", "saveAs", "name", "database", "isNamedDrawing")


def _clear_old(name):
    """第 2/3 步：清空所有已导入的 PyArxCore 模块，返回被摘掉的模块名列表。"""
    cache = sys.modules.setdefault("__pyarx_old__", [])
    removed = []
    for k in list(sys.modules.keys()):
        if k == "PyArxCore" or k.startswith("PyArxCore.") or k == name:
            old = sys.modules.pop(k, None)
            if old is not None:
                cache.append(old)
                removed.append(k)
    return removed


def main():
    if OUTDIR not in sys.path:
        sys.path.insert(0, OUTDIR)
    name = open(MODTXT, encoding="utf-8").read().strip()

    rep = {"new_module": name, "ok": False}
    rep["cleared"] = _clear_old(name)          # 先清空
    rep["alive_old"] = len(sys.modules.get("__pyarx_old__", []))

    m = importlib.import_module(name)           # 再导入（干净初始化）
    sys.modules["PyArxCore"] = m
    rep["alias_file"] = getattr(m, "__file__", "")

    dm = m.Ap.PyApp.documentManager
    rep["dm_missing"] = [a for a in NEED_DM if not hasattr(dm, a)]
    doc = dm.curDocument
    rep["doc_missing"] = [a for a in NEED_DOC if not hasattr(doc, a)]
    rep["doc_name"] = doc.name
    rep["count"] = dm.count

    rep["ok"] = (
        rep["alias_file"].endswith(name + ".pyd")
        and not rep["dm_missing"]
        and not rep["doc_missing"]
    )
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"ok": False, "exception": traceback.format_exc()}
    with open(RESULT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[hotreload] " + json.dumps(rep, ensure_ascii=False)[:400])
