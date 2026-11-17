"""reload_same.py —— 释放旧模块并重新加载（同名热替换路径）。

与 reload.py（换新名字）互补：这条路径让模块名保持不变，代价是必须先
把旧 DLL 从进程里释放（FreeLibrary），否则文件被映射、无法覆盖也无法重载。

流程：
  1. 记下旧模块的 pyd 路径；
  2. 从 sys.modules 摘掉 PyArxCore 及其子模块；
  3. 删掉 builtins 里所有 __pybind11_internals*__ 条目，
     使新加载的 DLL 建一套全新的类型注册表（否则报 "type X is already registered"）；
  4. FreeLibrary 旧 DLL（循环到句柄失效）；
  5. import 新模块（同名字或新名字，由 module.txt 指定）。

注意：释放后旧模块对象不可再被访问（其代码段已卸载）。脚本、ARX 若仍持有
旧对象引用，访问即崩溃 —— 所以先摘 sys.modules、再释放。
"""

import ctypes
import importlib
import json
import os
import sys
import traceback

OUTDIR = r"G:\PyArx\X64\Release"
MODTXT = r"G:\PyArx\tools\hotbuild\module.txt"
RESULT = r"G:\PyArx\tools\hotbuild\reload_out.json"


def purge_internals(builtins_mod):
    keys = [k for k in vars(builtins_mod) if k.startswith("__pybind11_internals")]
    for k in keys:
        try:
            delattr(builtins_mod, k)
        except Exception:
            pass
    return keys


def unload(path):
    k32 = ctypes.WinDLL("kernel32", use_last_error=True)
    k32.GetModuleHandleW.argtypes = [ctypes.c_wchar_p]
    k32.GetModuleHandleW.restype = ctypes.c_void_p
    k32.FreeLibrary.argtypes = [ctypes.c_void_p]
    k32.FreeLibrary.restype = ctypes.c_bool
    path_w = os.path.normcase(path)
    freed = 0
    for _ in range(8):
        h = k32.GetModuleHandleW(path_w)
        if not h:
            break
        if not k32.FreeLibrary(h):
            break
        freed += 1
    still = k32.GetModuleHandleW(path_w)
    return {"freed": freed, "stillLoaded": bool(still)}


def main():
    if OUTDIR not in sys.path:
        sys.path.insert(0, OUTDIR)
    target = open(MODTXT, encoding="utf-8").read().strip()
    rep = {"target": target, "ok": False}

    old = sys.modules.get("PyArxCore")
    old_path = os.path.abspath(getattr(old, "__file__", "") or "")
    rep["old_file"] = old_path

    # 1) 摘掉模块引用（含子模块）
    for k in list(sys.modules):
        if k == "PyArxCore" or k.startswith("PyArxCore."):
            sys.modules.pop(k, None)
    if old_path:
        stem = os.path.splitext(os.path.basename(old_path))[0]
        for k in list(sys.modules):
            if k == stem or k.startswith(stem + "."):
                sys.modules.pop(k, None)
    del old

    # 2) 清 pybind11 internals，避免类型重复注册
    import builtins
    rep["purged"] = purge_internals(builtins)

    # 3) 释放旧 DLL
    if old_path and os.path.exists(old_path):
        rep["unload"] = unload(old_path)

    # 4) 重新加载
    m = importlib.import_module(target)
    sys.modules["PyArxCore"] = m
    rep["alias_file"] = getattr(m, "__file__", "")

    db = m.Db.curDb()
    rep["db_file"] = db.getFilename()
    rep["entities"] = len(db.objectIds("AcDbEntity"))
    rep["ok"] = True
    return rep


if __name__ == "__main__":
    try:
        rep = main()
    except Exception:
        rep = {"ok": False, "exception": traceback.format_exc()}
    with open(RESULT, "w", encoding="utf-8") as f:
        json.dump(rep, f, ensure_ascii=False, indent=1)
    print("[reload_same] " + json.dumps(rep, ensure_ascii=False)[:400])
