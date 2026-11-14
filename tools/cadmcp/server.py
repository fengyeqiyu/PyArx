"""cadmcp —— 让任意 MCP 客户端直接操作运行中的 AutoCAD（经由 PyArx）。

启动：python server.py           （stdio，MCP 协议）
工具：
  pyarx_run(code, timeout_sec, doc)  在 acad.exe 内执行 PyArx 代码
  pyarx_run_file(path, timeout_sec, doc)  在 acad.exe 内执行一个 .py 文件
  pyarx_api_search(query, limit)     检索 PyArxCore 全量 API 清单
  pyarx_guide()                      返回已实测的 PyArx 用法要点
  pyarx_hotbuild(tag)                热编译 PyArxCore 并在运行中的 CAD 内热替换
  pyarx_build(target)                正式编译基础 pyd（PyArxCore.pyd，arx 启动加载的那个）
  pyarx_restart_cad()                存盘 → 退出 → 重启 acad → 等 COM 就绪
  pyarx_save / pyarx_saveas          存盘（COM Document.Save / db.saveAs，无弹窗）
  pyarx_projects(query)              返回全部工程/目录的职责地图（docs/PYARX_PROJECTS.md）
  xml_render(path, out, size, as_image)  把 XML 界面离屏渲染成 PNG（不开窗口，AI/CI 用）
  xml_layout(path, max_lines)         校验 XML 并返回布局树文本（几何问题在这里最清楚）
"""

import json
import os
import subprocess
import sys
import threading
import time
import uuid
from datetime import datetime

BASE = r"G:\PyArx\tools\cadmcp"
API_JSON = r"G:\PyArx\docs\pyarx_api_raw.json"
GUIDE = r"G:\PyArx\docs\PYARX_USAGE.md"
PROJECTS = r"G:\PyArx\docs\PYARX_PROJECTS.md"
REQ = os.path.join(BASE, "req.json")
# bridge 只有一个 req/res 插槽，并发调用会互相覆盖/抢占结果，必须串行化
RUN_LOCK = threading.Lock()
RES = os.path.join(BASE, "res.json")
CAD_COM = os.path.join(BASE, "cad_com.ps1")        # COM 通道：保存/退出/文档列表
CAD_EXE = r"C:\Program Files\Autodesk\AutoCAD 2024\acad.exe"
MSBUILD = r"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
SLN = r"G:\PyArx\PyArx.sln"
CONSOLE = r"C:\Program Files\Autodesk\AutoCAD 2024\accoreconsole.exe"

# 崩溃守卫：检测并关闭 AutoCAD 错误报告/断言框（纯 Win32，不依赖 CAD 存活）
sys.path.insert(0, BASE)
try:
    import cad_guard
except Exception:      # 守卫不可用时退化为空实现
    cad_guard = None
GUARD_LOG = r"G:\PyArx\log\cad_guard.log"


def guard_preflight():
    """每次发命令前清掉挡住 COM/SendCommand 的崩溃/报错窗口。"""
    if cad_guard is None:
        return {"available": False}
    try:
        dialogs = cad_guard.scan()
        closed = []
        if dialogs:
            closed = cad_guard.close()
            with open(GUARD_LOG, "a", encoding="utf-8") as f:
                f.write("[%s] preflight closed=%s\n" % (
                    datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                    json.dumps([c["title"] for c in closed], ensure_ascii=False)))
        return {"available": True, "closed": closed, "acad_pids": cad_guard.acad_pids()}
    except Exception as e:
        return {"available": False, "error": str(e)}

TOOLS = [
    {
        "name": "pyarx_run",
        "description": (
            "在运行中的 AutoCAD 进程内执行 Python/PyArx 代码并返回 result/out/error。"
            "代码里已注入 P = PyArxCore；赋值给 result 的变量会被回传（JSON 可序列化）。"
            "推荐范式：db = P.Db.curDb(); with P.Db.PyDbTrans() as tr: ...; tr.addEntity(pl)"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "code": {"type": "string", "description": "要执行的 Python 源码"},
                "timeout_sec": {"type": "integer", "default": 90},
                "doc": {"type": "string", "description": "目标图纸名片段（可选，默认活动图）"},
            },
            "required": ["code"],
        },
    },
    {
        "name": "pyarx_run_file",
        "description": (
            "把一个 .py 脚本文件在运行中的 AutoCAD 内执行（源码经 bridge 注入 acad.exe）。"
            "与 pyarx_run 等价，只是代码来自文件；脚本里赋值给 result 的变量会回传。"
            "适合把一段可复用的操作落盘留档后再执行。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "脚本绝对路径，UTF-8"},
                "timeout_sec": {"type": "integer", "default": 90},
                "doc": {"type": "string", "description": "目标图纸名片段（可选）"},
            },
            "required": ["path"],
        },
    },
    {
        "name": "pyarx_api_search",
        "description": "在 PyArxCore 全量 API 清单里检索类/成员名，返回匹配的命名空间与类。",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string"},
                "limit": {"type": "integer", "default": 20},
            },
            "required": ["query"],
        },
    },
    {
        "name": "pyarx_guide",
        "description": "返回 PyArx 实测用法要点（可用入口、已知坑、推荐范式）。",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "pyarx_hotbuild",
        "description": (
            "热编译 PyArxCore：改完 C++ 源码后，增量编译出新的 pyd 并在运行中的 AutoCAD "
            "内热替换生效，无需重启 CAD。增量构建约 13 秒（首次切到 hot 模式或改公共头文件 "
            "才需全量约 9 分钟）。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "tag": {"type": "string", "description": "版本后缀，默认时间戳"},
                "timeout_sec": {"type": "integer", "default": 600},
            },
        },
    },
    {
        "name": "pyarx_docs",
        "description": "列出 CAD 当前打开的全部文档（名称、标题、是否活动、是否只读）。",
        "inputSchema": {
            "type": "object",
            "properties": {"timeout_sec": {"type": "integer", "default": 60}},
        },
    },
    {
        "name": "pyarx_save",
        "description": (
            "保存 CAD 当前打开的全部文档（默认走 COM：Document.Save() 同步落盘、"
            "不弹任何对话框；COM 不可用时自动退回 PyArx 通道 db->saveAs/dxfOut）。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {"timeout_sec": {"type": "integer", "default": 90}},
        },
    },
    {
        "name": "pyarx_build",
        "description": (
            "正式编译基础模块（默认 target=PyArxCore），产出 X64\\Release\\PyArxCore.pyd。"
            "arx 启动时加载的就是它；改完 C++ 想彻底生效（而不是内存热替换）时用它 + pyarx_restart_cad。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "target": {"type": "string", "default": "PyArxCore"},
                "timeout_sec": {"type": "integer", "default": 600},
            },
        },
    },
    {
        "name": "pyarx_restart_cad",
        "description": (
            "重启 AutoCAD：先 COM 存盘并 Quit，确认进程退出后重新启动 acad.exe，"
            "再轮询 COM 直到 AutoCAD.Application 可连接（返回就绪耗时与文档列表）。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "save_first": {"type": "boolean", "default": True},
                "wait_ready_sec": {"type": "integer", "default": 120},
            },
        },
    },
    {
        "name": "pyarx_saveas",
        "description": (
            "把当前活动文档另存为指定路径（走 _.SAVEAS 2018，可用正则替换原名）。"
            "用于未命名文档，或需要改名的场合；已命名文档直接用 pyarx_save 即可。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "目标绝对路径，例如 C:/temp/x.dwg"},
                "timeout_sec": {"type": "integer", "default": 90},
            },
            "required": ["path"],
        },
    },
    {
        "name": "pyarx_quit_cad",
        "description": (
            "关闭 AutoCAD：默认走 COM，先把脏文档逐个 Save() 再 Application.Quit()，全程无弹窗；"
            "save_first=false 则直接退出。COM 不可用时退回 _.QUIT 命令通道。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "save_first": {"type": "boolean", "default": True},
                "timeout_sec": {"type": "integer", "default": 90},
            },
        },
    },
    {
        "name": "pyarx_autoload",
        "description": (
            "管理 CAD 启动自加载 PyArx 插件（注册 PYCArxEngine.arx 到 "
            "HKCU\\...\\Applications\\PyArx，LOADCTRLS=0x2）。mode=install|status|remove。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "mode": {"type": "string", "default": "install",
                         "enum": ["install", "status", "remove"]},
                "loader": {"type": "string", "description": "arx 全路径，默认 X64\\Release\\PyArxEngine.arx"},
            },
        },
    },
    {
        "name": "pyarx_com",
        "description": (
            "直接走 COM 操作 AutoCAD（不依赖 PyArx 插件是否加载）："
            "action=docs 列文档 / save 存盘 / quit 退出。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "action": {"type": "string", "default": "docs",
                           "enum": ["docs", "save", "quit"]},
                "timeout_sec": {"type": "integer", "default": 90},
            },
        },
    },
    {
        "name": "pyarx_projects",
        "description": (
            "返回 PyArx 全部工程/目录的职责地图（每个项目的作用、产物、依赖、关键文件、"
            "边界与坑、改动落点）。改代码前先查这里确定归属。query 为空返回全文，"
            "非空则只返回包含该关键词的章节。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "章节关键词，如 PyMfcXml / 控件 / 构建"},
            },
        },
    },
    {
        "name": "pyarx_headless",
        "description": (
            "无头快速通道：accoreconsole + PyArxEngine.crx，在后台进程里跑 PyArx 脚本，"
            "一轮约 5-8 秒，不碰正在运行的 CAD（探测签名/危险建模首选用它）。"
            "脚本需自己 import PyArxCore as P，并把 JSON 结果写到 out 指定路径。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "py": {"type": "string", "description": "脚本绝对路径"},
                "out": {"type": "string", "description": "脚本写出的 JSON 结果路径（可选，默认同名 _out.json）"},
                "dwg": {"type": "string", "description": "模板图纸（可选，默认 C:\\temp\\pyarx_db_save_test.dwg）"},
                "timeout_sec": {"type": "integer", "default": 60},
            },
            "required": ["py"],
        },
    },
    {
        "name": "pyarx_guard",
        "description": (
            "崩溃/弹窗守卫：检测并关闭 AutoCAD 错误报告(CER)、Assertion failed、Fatal Error "
            "这类挡住 COM/SendCommand 的窗口，并回报 acad.exe 是否存活。"
            "action=status 看状态；scan 只列出；close 发 WM_CLOSE 关掉（不杀进程）。"
            "pyarx_run 已内置同样的前置清理。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "action": {"type": "string", "enum": ["status", "scan", "close"], "default": "status"},
                "dry_run": {"type": "boolean", "default": False},
            },
        },
    },
    {
        "name": "xml_render",
        "description": (
            "把一份 XML 界面离屏渲染成 PNG（不开窗口、不建 MFC 真控件、不需要桌面会话）。"
            "画法与设计器画布/预览器同一份，所以产物就是“这份 XML 渲染出来长什么样”的标准图。"
            "as_image=true 时把 PNG 以 image 内容一并返回（模型可直接看图）；默认只回路径，省 token。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "XML 绝对路径"},
                "out": {"type": "string", "description": "输出 PNG 路径（缺省与 XML 同目录同名）"},
                "size": {"type": "string", "description": "像素尺寸 WxH，缺省按根节点 + 20px 边距自动算"},
                "as_image": {"type": "boolean", "default": False},
                "timeout_sec": {"type": "integer", "default": 60},
            },
            "required": ["path"],
        },
    },
    {
        "name": "xml_layout",
        "description": (
            "校验 XML 并返回布局结果文本（每个节点的 tag/name 与声明、实际算出的 x/y/w/h）。"
            "看图之前先用它确认几何：越界、宽 0、被内容撑开的问题在这里看得最清楚。"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {"type": "string", "description": "XML 绝对路径"},
                "max_lines": {"type": "integer", "default": 400, "description": "返回行数上限（树很深时截断）"},
                "timeout_sec": {"type": "integer", "default": 60},
            },
            "required": ["path"],
        },
    },
]


def log(msg):
    sys.stderr.write("[cadmcp] " + str(msg) + "\n")
    sys.stderr.flush()


def com_call(action, timeout=90, path=""):
    """走 COM（cad_com.ps1）保存/退出/文档列表/开关图纸：不弹窗、不依赖 PyArx 是否已加载。"""
    if not os.path.isfile(CAD_COM):
        return {"ok": False, "error": "cad_com.ps1 missing: " + CAD_COM}
    cmd = ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass",
           "-File", CAD_COM, "-Action", action]
    if path:
        cmd += ["-Path", path]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout + 30)
    raw = (proc.stdout or "").strip()
    if not raw:
        return {"ok": False, "error": "no output from cad_com.ps1",
                "stderr": (proc.stderr or "")[-600:]}
    try:
        out = json.loads(raw.splitlines()[-1])
    except Exception:
        return {"ok": False, "error": "bad json from cad_com.ps1", "raw": raw[-600:]}
    out["channel"] = "com"
    return out
def call_run(args):
    with RUN_LOCK:
        return _call_run(args)


def _call_run(args):
    code = args.get("code", "")
    timeout = int(args.get("timeout_sec", 90))
    doc = args.get("doc", "")
    # 先清掉崩溃/报错弹框；没有 acad 进程时直接回报，避免"SendCommand rejected"这种含糊错误
    guard = guard_preflight()
    if guard.get("available") and not guard.get("acad_pids"):
        return {"ok": False, "error": "CAD not running",
                "detail": "acad.exe 不存在（可能刚崩溃/已退出）；先 pyarx_restart_cad",
                "guard": guard}
    rid = uuid.uuid4().hex[:12]
    try:
        if os.path.exists(RES):
            os.remove(RES)
    except Exception as ex:
        log("res cleanup failed: " + repr(ex))
    with open(REQ, "w", encoding="utf-8") as f:
        json.dump({"id": rid, "code": code}, f, ensure_ascii=False)
    cmd = ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass",
           "-File", os.path.join(BASE, "cad_send.ps1"),
           "-PyFile", os.path.join(BASE, "bridge.py"),
           "-TimeoutSec", str(timeout + 30)]
    if doc:
        cmd += ["-DocName", doc]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout + 60)
    raw = (proc.stdout or "").strip()
    if not raw:
        return {"ok": False, "error": "no output from CAD bridge", "stderr": (proc.stderr or "")[-800:]}
    try:
        payload = json.loads(raw)
    except Exception:
        return {"ok": False, "error": "bad json from bridge", "raw": raw[-800:]}
    got = payload.get("id")
    if got and got != rid:
        return {"ok": False, "error": "stale bridge result (id mismatch: %s != %s)" % (got, rid)}
    if payload.get("error"):
        return {"ok": False, "error": payload["error"], "out": payload.get("out", "")}
    return {"ok": True, "result": payload.get("result"), "out": payload.get("out", "")}


def call_api_search(args):
    q = (args.get("query") or "").lower()
    limit = int(args.get("limit", 20))
    if not q:
        return {"ok": False, "error": "empty query"}
    with open(API_JSON, encoding="utf-8") as f:
        data = json.load(f)
    hits = []
    for ns, classes in data.items():
        if not isinstance(classes, dict):
            continue
        for cls, members in classes.items():
            if q in cls.lower():
                hits.append({"ns": ns, "class": cls, "members": (members or [])[:80]})
            elif isinstance(members, list):
                m = [x for x in members if q in str(x).lower()]
                if m:
                    hits.append({"ns": ns, "class": cls, "members": m[:40]})
    return {"ok": True, "count": len(hits), "hits": hits[:limit]}


def call_guide():
    if os.path.exists(GUIDE):
        with open(GUIDE, encoding="utf-8") as f:
            return {"ok": True, "guide": f.read()}
    return {"ok": False, "error": "guide missing: " + GUIDE}


def call_run_file(args):
    path = (args.get("path") or "").strip()
    if not path or not os.path.isfile(path):
        return {"ok": False, "error": "script not found: " + path}
    try:
        with open(path, encoding="utf-8") as f:
            code = f.read()
    except Exception as e:
        return {"ok": False, "error": "read failed: %r" % (e,)}
    out = call_run({"code": code,
                    "timeout_sec": args.get("timeout_sec", 90),
                    "doc": args.get("doc", "")})
    out["script"] = path
    return out


def call_projects(args):
    if not os.path.exists(PROJECTS):
        return {"ok": False, "error": "projects map missing: " + PROJECTS}
    with open(PROJECTS, encoding="utf-8") as f:
        text = f.read()
    q = (args.get("query") or "").strip().lower()
    if not q:
        return {"ok": True, "path": PROJECTS, "text": text}
    # 按二级标题切章，命中标题或正文的章节返回
    parts = text.split("\n## ")
    hits = [parts[0]] if q in parts[0].lower() else []
    for p in parts[1:]:
        title = p.split("\n", 1)[0]
        if q in p.lower():
            hits.append("## " + p)
    if not hits:
        return {"ok": True, "path": PROJECTS, "query": q, "text": "", "note": "no section matched"}
    return {"ok": True, "path": PROJECTS, "query": q, "text": "\n".join(hits)}


def call_hotbuild(args):
    """编译新 pyd + 在 CAD 内热替换。"""
    hot_ps1 = r"G:\PyArx\tools\hotbuild\hotbuild.ps1"
    reload_py = r"G:\PyArx\tools\hotbuild\reload.py"
    reload_out = r"G:\PyArx\tools\hotbuild\reload_out.json"
    tag = (args.get("tag") or "").strip()
    timeout = int(args.get("timeout_sec", 600))

    cmd = ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", hot_ps1]
    if tag:
        cmd += ["-Tag", tag]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    build_out = (proc.stdout or "").strip()
    mod = ""
    for line in build_out.splitlines():
        if line.startswith("MODULE="):
            mod = line.split("=", 1)[1].strip()
    if not mod:
        return {"ok": False, "stage": "build", "out": build_out[-800:],
                "stderr": (proc.stderr or "")[-400:]}

    if os.path.exists(reload_out):
        os.remove(reload_out)
    run = call_run({
        "code": "import runpy\n"
                "runpy.run_path(r'G:/PyArx/tools/hotbuild/reload.py', run_name='__main__')\n"
                "result = 'done'",
        "timeout_sec": 120,
    })
    rep = {}
    if os.path.exists(reload_out):
        try:
            with open(reload_out, encoding="utf-8") as f:
                rep = json.load(f)
        except Exception:
            rep = {}
    return {"ok": bool(rep.get("ok")), "module": mod,
            "reload": rep, "build_log_tail": build_out[-300:], "bridge_ok": run.get("ok")}


DOCS_CODE = """
dm = P.Ap.PyApp.documentManager
info = []
for i in range(dm.count):
    d = dm.documentAt(i)
    info.append({"name": d.name, "title": d.docTitle,
                 "active": bool(d.isActive), "readOnly": bool(d.isReadOnly)})
result = {"count": dm.count, "docs": info}
"""

SAVE_CODE = """
dm = P.Ap.PyApp.documentManager
saved = []
for i in range(dm.count):
    d = dm.documentAt(i)
    saved.append({"name": d.name, "status": str(d.save())})
result = {"count": dm.count, "saved": saved}
"""

QUIT_CODE = """
dm = P.Ap.PyApp.documentManager
saved = []
if __SAVE_FIRST__:
    for i in range(dm.count):
        d = dm.documentAt(i)
        saved.append({"name": d.name, "status": str(d.save())})
quitStatus = str(dm.quit())
result = {"saved": saved, "quit": quitStatus}
"""


def call_docs(args):
    out = com_call("docs", timeout=args.get("timeout_sec", 60))
    if out.get("ok"):
        return out
    out["fallback"] = call_run({"code": DOCS_CODE, "timeout_sec": args.get("timeout_sec", 60)})
    return out


def call_save(args):
    # 首选 COM：Document.Save() 同步落盘、无模态框、不依赖 PyArx
    out = com_call("save", timeout=args.get("timeout_sec", 90))
    if out.get("ok") and all(x.get("ok") for x in out.get("saved", [])):
        return out
    # COM 不可用（CAD 模态/未注册）时退回 PyArx 通道（save() 已改 db->saveAs/dxfOut）
    out["fallback"] = call_run({"code": SAVE_CODE, "timeout_sec": args.get("timeout_sec", 90)})
    return out


def call_saveas(args):
    path = (args.get("path") or "").strip()
    if not path:
        return {"ok": False, "error": "path required"}
    # 走 AcDbDatabase::saveAs：同步写盘、不弹"另存为"、不问格式（_.SAVEAS 会卡在格式提示上）
    code = ("d = P.Ap.PyApp.documentManager.curDocument\n"
            "st = str(d.database.saveAs(r'%s'))\n"
            "result = {'status': st, 'path': r'%s'}\n") % (path, path)
    out = call_run({"code": code, "timeout_sec": args.get("timeout_sec", 90)})
    if out.get("ok"):
        out["fileExists"] = os.path.exists(path)
        out["size"] = os.path.getsize(path) if os.path.exists(path) else 0
    return out


def call_quit_cad(args):
    # 首选 COM：先对脏文档逐个 Save()，再 Application.Quit()，全程无弹窗
    out = com_call("quit", timeout=args.get("timeout_sec", 90))
    if out.get("ok") and out.get("quit") == "sent":
        return out
    save_first = "True" if args.get("save_first", True) else "False"
    code = QUIT_CODE.replace("__SAVE_FIRST__", save_first)
    out["fallback"] = call_run({"code": code, "timeout_sec": args.get("timeout_sec", 90)})
    return out


def _acad_pids():
    proc = subprocess.run(
        ["powershell.exe", "-NoProfile", "-Command",
         "(Get-Process acad -ErrorAction SilentlyContinue | ForEach-Object { $_.Id }) -join ','"],
        capture_output=True, text=True, timeout=30)
    raw = (proc.stdout or "").strip()
    return [int(x) for x in raw.split(",") if x.strip().isdigit()]


def call_build(args):
    """正式编译基础 PyArxCore.pyd（arx 启动时加载的就是它，热模块只在内存里顶替）。"""
    target = (args.get("target") or "PyArxCore").strip()
    timeout = int(args.get("timeout_sec", 600))
    proc = subprocess.run(
        [MSBUILD, SLN, "/t:" + target, "/p:Configuration=Release", "/p:Platform=x64",
         "/p:VcpApplocalDeps=false", "/m", "/v:m"],
        capture_output=True, text=True, timeout=timeout)
    pyd = os.path.join(r"G:\PyArx\X64\Release", target + ".pyd")
    out = {"ok": proc.returncode == 0, "target": target, "exit": proc.returncode,
           "pyd": pyd, "exists": os.path.exists(pyd)}
    if out["exists"]:
        st = os.stat(pyd)
        out["bytes"] = st.st_size
        out["mtime"] = datetime.fromtimestamp(st.st_mtime).strftime("%Y-%m-%d %H:%M:%S")
    out["log_tail"] = (proc.stdout or "").strip()[-600:]
    return out


def call_restart_cad(args):
    """关掉 CAD（先 COM 存盘）→ 重新启动 acad.exe → 等 COM 就绪。"""
    save_first = bool(args.get("save_first", True))
    wait_ready = int(args.get("wait_ready_sec", 120))
    steps = {}
    if _acad_pids():
        if save_first:
            steps["save"] = com_call("save", timeout=90)
        steps["quit"] = com_call("quit", timeout=90)
        for _ in range(20):
            if not _acad_pids():
                break
            time.sleep(1)
    pids = _acad_pids()
    if pids:
        # 还剩实例：不硬杀用户进程，把现场交回调用方决定
        return {"ok": False, "stage": "quit", "pids": pids, "steps": steps,
                "error": "AutoCAD still running after quit"}
    subprocess.Popen([CAD_EXE], close_fds=True)
    started = time.time()
    steps["start"] = "acad.exe launched"
    deadline = started + wait_ready
    while time.time() < deadline:
        d = com_call("docs", timeout=20)
        if d.get("ok"):
            steps["ready_after_sec"] = int(time.time() - started)
            return {"ok": True, "docs": d, "pids": _acad_pids(), "steps": steps}
        time.sleep(3)
    return {"ok": False, "stage": "wait_ready", "pids": _acad_pids(), "steps": steps,
            "error": "AutoCAD COM not ready in %ds" % wait_ready}


def call_autoload(args):
    """让 CAD 启动时自动加载插件（注册表 Applications 项，LOADCTRLS=0x2）。"""
    ps1 = os.path.join(BASE, "..", "autoload", "set_autoload.ps1")
    ps1 = os.path.abspath(ps1)
    if not os.path.isfile(ps1):
        return {"ok": False, "error": "set_autoload.ps1 missing: " + ps1}
    cmd = ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", ps1]
    mode = (args.get("mode") or "install").strip()
    if mode == "status":
        cmd.append("-Status")
    elif mode == "remove":
        cmd.append("-Remove")
    if args.get("loader"):
        cmd += ["-Loader", args["loader"]]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
    raw = (proc.stdout or "").strip()
    try:
        out = json.loads(raw.splitlines()[-1])
    except Exception:
        out = {"ok": False, "error": "bad output", "raw": raw[-500:],
               "stderr": (proc.stderr or "")[-300:]}
    out["mode"] = mode
    return out


def call_headless(args):
    """无头通道：accoreconsole + PyArxEngine.crx，一轮约 5-8 秒，不碰用户 CAD。

    适合：探测 API 签名、跑危险建模、批量回归。脚本要自己 import PyArxCore（PYRUNFILE 不注入 P），
    并把结果写成 JSON 文件（建议用 out 参数指定路径）。
    """
    import shutil
    py = (args.get("py") or "").strip()
    if not py or not os.path.isfile(py):
        return {"ok": False, "error": "script not found: " + py}
    out_json = (args.get("out") or "").strip() or os.path.splitext(py)[0] + "_out.json"
    dwg_src = (args.get("dwg") or "").strip() or r"C:\temp\pyarx_db_save_test.dwg"
    if not os.path.isfile(dwg_src):
        return {"ok": False, "error": "template dwg missing: " + dwg_src}
    timeout = int(args.get("timeout_sec", 60))
    tag = uuid.uuid4().hex[:6]
    dwg = os.path.join(r"C:\temp", "headless_%s.dwg" % tag)
    scr = os.path.join(r"C:\temp", "headless_%s.scr" % tag)
    log = os.path.join(r"C:\temp", "headless_%s.log" % tag)
    shutil.copyfile(dwg_src, dwg)
    with open(scr, "w", encoding="utf-8") as f:
        f.write('(setvar "SECURELOAD" 0)\n')
        f.write('(arxload "G:/PyArx/X64/Release/PyArxEngine.crx")\n')
        f.write('(command "PYRUNFILE" "%s")\n' % py.replace("\\", "/"))
    try:
        if os.path.exists(out_json):
            os.remove(out_json)
    except Exception:
        pass
    t0 = time.time()
    proc = subprocess.run([CONSOLE, "/i", dwg, "/s", scr],
                          capture_output=True, timeout=timeout)
    elapsed = round(time.time() - t0, 1)
    out = {"ok": True, "elapsed_sec": elapsed, "exit": proc.returncode,
           "script": py, "out_json": out_json}
    if os.path.exists(out_json):
        try:
            with open(out_json, encoding="utf-8") as f:
                out["python_out"] = json.load(f)
        except Exception as e:
            out["out_read_error"] = repr(e)
    else:
        out["ok"] = False
        out["error"] = "脚本没有写出结果 JSON（可能语法错误或引擎未就绪）"
    stdout = (proc.stdout or b"").decode("utf-16-le", errors="ignore")
    out["console_tail"] = stdout.strip()[-800:]
    return out


def call_guard(args):
    """崩溃守卫：status/scan/close。close 只发 WM_CLOSE，不 kill 进程。"""
    action = (args.get("action") or "status").strip()
    if cad_guard is None:
        return {"ok": False, "error": "cad_guard 未加载"}
    if action == "scan":
        return {"ok": True, "dialogs": cad_guard.scan()}
    if action == "close":
        closed = cad_guard.close(dry_run=bool(args.get("dry_run")))
        if closed:
            with open(GUARD_LOG, "a", encoding="utf-8") as f:
                f.write("[%s] close=%s\n" % (
                    datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                    json.dumps([c["title"] for c in closed], ensure_ascii=False)))
        return {"ok": True, "closed": closed, "acad_pids": cad_guard.acad_pids()}
    return {"ok": True, "acad_pids": cad_guard.acad_pids(), "dialogs": cad_guard.scan()}


# ---------------------------------------------------------------------------
# XML 界面离屏渲染：Viewer --render / --check --dump
# ---------------------------------------------------------------------------
VIEWER = r"G:\PyArx\X64\Release\Viewer.exe"


def _decode_console(raw):
    """Viewer 的 wprintf 走 ANSI 代码页（GBK），按序试 UTF-8/GBK，别把中文吃成乱码。"""
    for enc in ("utf-8", "gbk"):
        try:
            return raw.decode(enc)
        except Exception:
            pass
    return raw.decode("utf-8", errors="replace")


def _viewer_env():
    env = dict(os.environ)
    env["PYARX_D2D_RT"] = "software"   # 离屏不碰显卡：远程/无桌面机器也能出图
    env["XML_D2D_AA"] = "1"
    return env


def call_xml_render(args):
    path = (args.get("path") or "").strip()
    if not path or not os.path.isfile(path):
        return {"ok": False, "error": "xml not found: " + path}
    if not os.path.isfile(VIEWER):
        return {"ok": False, "error": "Viewer 不存在（先编译 Release）: " + VIEWER}
    out = (args.get("out") or "").strip()
    if not out:
        # 默认落到 docs\ —— 截图产物归一处，别把 PNG 撒进 examples/ui
        name = os.path.splitext(os.path.basename(path))[0] + ".png"
        out = os.path.join(r"G:\PyArx\docs", name)
    cmd = [VIEWER, "--render", out]
    size = (args.get("size") or "").strip()
    if size:
        cmd += ["--size", size]
    cmd.append(path)

    t0 = time.time()
    proc = subprocess.run(cmd, capture_output=True, timeout=int(args.get("timeout_sec", 60)),
                          env=_viewer_env())
    elapsed = round(time.time() - t0, 2)
    text = _decode_console(proc.stdout or b"")
    exists = os.path.isfile(out)
    result = {"ok": bool(exists and proc.returncode == 0), "png": out,
              "exists": exists, "elapsed_sec": elapsed, "exit": proc.returncode,
              "viewer_out": text.strip()[-400:]}
    if exists:
        result["bytes"] = os.path.getsize(out)
        try:
            # PNG IHDR：宽高在 16..23 字节，不用拉 Pillow
            with open(out, "rb") as f:
                head = f.read(33)
            if head[:8] == b"\x89PNG\r\n\x1a\n":
                import struct
                w, h = struct.unpack(">II", head[16:24])
                result["width"], result["height"] = w, h
        except Exception:
            pass
    if args.get("as_image") and exists:
        import base64
        with open(out, "rb") as f:
            result["_images"] = [{"mimeType": "image/png",
                                  "data": base64.b64encode(f.read()).decode("ascii")}]
    return result


def call_xml_layout(args):
    path = (args.get("path") or "").strip()
    if not path or not os.path.isfile(path):
        return {"ok": False, "error": "xml not found: " + path}
    proc = subprocess.run([VIEWER, "--check", "--dump", path],
                          capture_output=True, timeout=int(args.get("timeout_sec", 60)),
                          env=_viewer_env())
    text = _decode_console(proc.stdout or b"")
    max_lines = int(args.get("max_lines", 400))
    lines = text.splitlines()
    truncated = len(lines) > max_lines
    return {"ok": True, "xml": path, "exit": proc.returncode,
            "lines": len(lines), "truncated": truncated,
            "text": "\n".join(lines[:max_lines])}


def watch_loop(interval=10):
    """后台看门狗：崩溃框自动关闭；acad 由有 → 无 记一条崩溃事件。"""
    if cad_guard is None:
        return
    had = bool(cad_guard.acad_pids())
    while True:
        try:
            dialogs = cad_guard.scan()
            if dialogs:
                cad_guard.close()
                with open(GUARD_LOG, "a", encoding="utf-8") as f:
                    f.write("[%s] watchdog closed=%s\n" % (
                        datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                        json.dumps([d["title"] for d in dialogs], ensure_ascii=False)))
            now = bool(cad_guard.acad_pids())
            if had and not now:
                with open(GUARD_LOG, "a", encoding="utf-8") as f:
                    f.write("[%s] acad.exe 进程消失（崩溃或退出）\n" %
                            datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
            had = now
        except Exception:
            pass
        time.sleep(interval)


def dispatch(name, args):
    if name == "pyarx_headless":
        return call_headless(args)
    if name == "pyarx_guard":
        return call_guard(args)
    if name == "xml_render":
        return call_xml_render(args)
    if name == "xml_layout":
        return call_xml_layout(args)
    if name == "pyarx_run":
        return call_run(args)
    if name == "pyarx_run_file":
        return call_run_file(args)
    if name == "pyarx_api_search":
        return call_api_search(args)
    if name == "pyarx_guide":
        return call_guide()
    if name == "pyarx_hotbuild":
        return call_hotbuild(args)
    if name == "pyarx_docs":
        return call_docs(args)
    if name == "pyarx_save":
        return call_save(args)
    if name == "pyarx_saveas":
        return call_saveas(args)
    if name == "pyarx_quit_cad":
        return call_quit_cad(args)
    if name == "pyarx_build":
        return call_build(args)
    if name == "pyarx_restart_cad":
        return call_restart_cad(args)
    if name == "pyarx_com":
        return com_call((args.get("action") or "docs").strip(), timeout=args.get("timeout_sec", 90))
    if name == "pyarx_autoload":
        return call_autoload(args)
    if name == "pyarx_projects":
        return call_projects(args)
    return {"ok": False, "error": "unknown tool: " + name}


def reply(obj):
    sys.stdout.write(json.dumps(obj, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def main():
    # 后台看门狗：自动关闭崩溃框、记录 acad 进程消失事件
    threading.Thread(target=watch_loop, kwargs={"interval": 10}, daemon=True).start()
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except Exception:
            continue
        mid = msg.get("id")
        method = msg.get("method", "")
        params = msg.get("params") or {}

        if method == "initialize":
            reply({"jsonrpc": "2.0", "id": mid, "result": {
                "protocolVersion": params.get("protocolVersion", "2024-11-05"),
                "capabilities": {"tools": {}},
                "serverInfo": {"name": "cadmcp", "version": "0.1.0"},
            }})
        elif method in ("notifications/initialized", "initialized"):
            continue
        elif method == "ping":
            reply({"jsonrpc": "2.0", "id": mid, "result": {}})
        elif method == "tools/list":
            reply({"jsonrpc": "2.0", "id": mid, "result": {"tools": TOOLS}})
        elif method == "tools/call":
            name = params.get("name")
            args = params.get("arguments") or {}
            try:
                out = dispatch(name, args)
            except Exception as e:
                out = {"ok": False, "error": repr(e)}
            content = [{"type": "text", "text": json.dumps(out, ensure_ascii=False)}]
            # 工具可以挂 _images（png 等）：文本之外再塞 image 内容，客户端能直接渲染
            for im in (out.pop("_images", None) or []):
                content.append({"type": "image", "mimeType": im.get("mimeType", "image/png"),
                                "data": im.get("data", "")})
            reply({"jsonrpc": "2.0", "id": mid, "result": {
                "content": content,
                "isError": not out.get("ok", False),
            }})
        else:
            reply({"jsonrpc": "2.0", "id": mid,
                   "error": {"code": -32601, "message": "method not found: " + method}})


if __name__ == "__main__":
    main()
