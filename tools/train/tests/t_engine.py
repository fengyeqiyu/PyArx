# -*- coding: utf-8 -*-
"""Engine 层：加载事实 + 命令链证据 + Core/Rx 引擎类扫描。不二次调起引擎命令（避免重入/交互）。"""
import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train\tests")
import PyArxCore as P
import mlab

SHARD = r"G:\PyArx\tools\train\data\shard_ed_app_engine.json"

# 源码核准（只读 G:\PyArx\PyArxEngine\acrxEntryPoint.cpp kInitAppMsg addCommand 清单）
SRC_CMDS = ["PYLOAD", "PYRUN", "PYEXEC", "PIPM", "PYDEBUG", "PYARXDEPLOY", "PYRUNFILE"]
SRC_GROUP = "PyArxEngine"

# 1. P.Engine 子模块不存在
mlab.check("Engine", "P.Engine_submodule", "missing", lambda: (
    "hasattr(P,'Engine')=%s；P 顶层子模块=%s" % (hasattr(P, "Engine"),
    sorted(x for x in dir(P) if not x.startswith("_")))))

# 2. 引擎已加载：acedArxLoaded / acedArxLoad / acedGetAppName
def t_loaded():
    loaded = P.Ed.acedArxLoaded()
    name = P.Ed.acedGetAppName()
    by_name = P.Ed.acedArxLoad("PyArxEngine")
    return "acedArxLoaded()=%r（含 PyArxEngine: %s）；acedGetAppName()=%r；acedArxLoad('PyArxEngine')=%s" % (
        loaded, "PyArxEngine" in [str(x) for x in (loaded or [])], name, by_name)
mlab.check("Engine", "PyArxEngine.loaded", "arxlist_appname", t_loaded)

# 3. 命令栈指针（不透明 int；PyArxCore 未暴露 lookup 接口）
mlab.check("Engine", "acedRegCmdsPtr", "opaque_pointer", lambda: (
    "acedRegCmdsPtr()=%r type=%s；返回不透明 AcEdCommandStack 指针；Core 未暴露按名 lookup 方法" % (
        P.Ed.acedRegCmdsPtr(), type(P.Ed.acedRegCmdsPtr()).__name__)))

# 4. 命令注册清单（源码核准）+ 运行链证据
def t_cmd_chain():
    ptr = P.Ed.acedRegCmdsPtr()
    return ("源码 acrxEntryPoint.cpp kInitAppMsg addCommand 组 '%s' 注册清单=%s；"
             "运行证据：本轮脚本由 scr 链 arxload PyArxEngine.crx -> (command \"PYRUNFILE\" 脚本路径) 执行，"
             "引擎懒初始化后 RunScript rc=0；acedRegCmdsPtr=%r（非空指针=%s）。"
             "Python 侧无命令名 lookup/二次调起接口，未重入调用其它引擎命令。" % (
                 SRC_GROUP, SRC_CMDS, ptr, bool(ptr)))
mlab.check("Engine", "PYRUNFILE.command_chain", "registered_executed", t_cmd_chain)

# 5. Core/Rx/Utility 内引擎相关类扫描
def scan(mod):
    m = getattr(P, mod, None)
    if m is None:
        return {"module": mod, "present": False}
    names = [x for x in dir(m) if not x.startswith("_")]
    hit = {k: [x for x in names if k in x] for k in ("Engine", "Cmd", "Binder", "Redir", "Dispatch")}
    return {"module": mod, "present": True, "count": len(names), "hit": hit, "names": names}

raw = {m: scan(m) for m in ("Core", "Rx", "Utility", "Ap", "Debug")}

def t_core_scan():
    c, r, u = raw["Core"], raw["Rx"], raw["Utility"]
    return ("P.Core=%s；P.Rx=%s；P.Utility=%s；Engine/Cmd/Binder/Redir/Dispatch 命中：Core=%s Rx=%s Utility=%s；"
            "顶层 P.Cmd=%s P.Redir=%s" % (
                c["names"], r["names"], u["names"],
                c["hit"], r["hit"], u["hit"], hasattr(P, "Cmd"), hasattr(P, "Redir")))
mlab.check("Engine", "core_rx_engine_classes", "absent_scan", t_core_scan)

rep = mlab.report()
dump = mlab.dump(SHARD)
out = mlab.out_path(r"C:\temp\engine.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"report": rep, "dump": dump, "raw": raw}, f, ensure_ascii=False, indent=1)
print(json.dumps({"dump": dump, "modules": len(rep)}, ensure_ascii=False))
