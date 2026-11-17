# -*- coding: utf-8 -*-
"""PyArx 本地模型训练台：训练进度可视化 + 本地模型对话测试。

只用标准库做 HTTP 服务，推理走已装的 torch/transformers/peft，不额外装依赖。
启动后浏览器打开 http://127.0.0.1:8765
"""

import copy
import json
import os
import re
import subprocess
import sys
import threading
import time
import traceback
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs

ROOT = r"G:\PyArx\tools\train"
BASE_MODEL = r"D:\models\qwen2.5-1.5b"
ADAPTER = os.path.join(ROOT, "out", "qwen1.5b-cad-lora")
LOG = os.path.join(ROOT, "logs", "train.log")
DATA = os.path.join(ROOT, "data", "sft_all.jsonl")
MATRIX = os.path.join(ROOT, "data", "module_matrix.json")
HERE = os.path.dirname(os.path.abspath(__file__))
CTX_LOG = os.path.join(ROOT, "logs", "context.log")
CADMCP = r"G:\PyArx\tools\cadmcp"
COM_PS1 = os.path.join(CADMCP, "cad_com.ps1")
COM_PY = r"C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe"
HEADLESS_STATS_PY = os.path.join(HERE, "_ctx_stats.py")
HEADLESS_STATS_OUT = r"C:\temp\ctx_stats_out.json"
CTX_DWG = os.environ.get("PYARX_CTX_DWG", r"C:\temp\pyarx_db_save_test.dwg")

LOCK_TIMEOUT_S = float(os.environ.get("PYARX_LOCK_TIMEOUT", "180"))
GEN_TIMEOUT_S = float(os.environ.get("PYARX_GEN_TIMEOUT", "120"))
CTX_TTL_MS = int(os.environ.get("PYARX_CTX_TTL", "45000"))

PORT = 8765

# 模型单实例：全局一把锁，拿不到锁 180 秒内回 503，生成超时 120 秒回 504
_chat_lock = threading.Lock()

SYS = ("你是 PyArx CAD 助手。绘图指令一律用 cad_* 工具调用，不要自己写绘图代码；坐标单位 mm，角度用度，图层不存在会自动创建。"
       "用户要脚本/代码时，照样输出 cad_* 工具调用——系统会据此生成 PyArx Python 代码，你不要在正文里手写代码，也不要劝用户改用别的做法。"
       "需要等分时给 cad_rect 填 vdivide=N（竖线 N 等分）或 hdivide=N（横线 N 等分），等分线坐标由工具层算，不要自己算竖线 x 值。"
       "cad_layer 只用于新建图层；要查某层上已有哪些图元用 cad_query。一次事务全有或全无；工具返回 ok/count/handles/warnings/errors，"
       "失败时要读 warnings 自纠而不是重试同一调用。"
       "被问 PyArxCore/PyArxEngine 的模块 API（类名、方法、签名、单位、返回形态）时，"
       "直接给实测确认的真实类名和方法名；名字不存在就明说不存在并给真实替代；不确定就说做不到，绝不编造工具名或参数名。")

TC_RE = re.compile(r"<tool_call>\s*(\{.*?\})\s*</tool_call>", re.S)
LINE_RE = re.compile(r"ep(\d+)\s+step(\d+)\s+loss=([0-9.]+)\s+(\d+)s")
TRAIN_RE = re.compile(r"rows=(\d+)\s+epochs=(\d+)\s+batch=(\d+)")

# 工具白名单与 schema：模型编出来的工具名/参数在这里被拦下，永远进不到 CAD
sys.path.insert(0, r"G:\PyArx\tools\cadmcp")
try:
    import cad_ops as _ops
    SCHEMAS = {s["name"]: s for s in _ops.SCHEMAS}
    ALLOWED = set(_ops.NAMES)
except Exception as _exc:
    SCHEMAS, ALLOWED = {}, set()
    print("validate disabled: %r" % (_exc,), flush=True)

_strikes = {"rejected": 0, "passed": 0}

FAKE_RE = re.compile(r"cad_[a-z_]+")


def _fake_tools(raw):
    """纯文本里出现的 cad_* 名字，只要不在白名单就是编的。

    实测模型被问"写脚本"时会答 `cad_entity`（17 个工具里没有），还给 cad_line 编
    出 start/end 参数（真实是 p1/p2）。纯文本回答原来直接 valid=True 放过去，
    这层必须拦。
    """
    if not ALLOWED:
        return []
    return sorted(set(FAKE_RE.findall(raw or "")) - ALLOWED)


def _script_for_tc(tc):
    """把 tool_call 翻成可直接在 CAD 里跑的 PyArx Python 代码。

    代码由工具层生成（cad_ops.script_for，生成后 ast.parse 自检），模型不碰源码。
    这样用户要"脚本"时看到的是真实可执行代码，而不是模型编出来的一段伪 API。
    """
    if not tc:
        return None
    name = str(tc.get("name") or "")
    short = name[4:] if name.startswith("cad_") else name
    args = tc.get("arguments") or {}
    try:
        return {"tool": name, "code": _ops.script_for(short, args), "err": ""}
    except Exception as exc:
        return {"tool": name, "code": "", "err": repr(exc)[:200]}

# 实测矩阵检索兜底：启动/首调时读入 module -> checks，按问题里的词做大小写不敏感子串匹配
_matrix = {"loaded": False, "modules": [], "by_name": {}}


def _load_matrix():
    if _matrix["loaded"]:
        return
    try:
        with open(MATRIX, encoding="utf-8") as f:
            data = json.load(f)
        for x in data:
            key = "%s.%s" % (x["layer"], x["module"])
            _matrix["modules"].append(key)
            _matrix["by_name"][x["module"].lower()] = {
                "key": key, "module": x["module"], "checks": x["checks"],
                "summary": x.get("summary") or {}}
    except Exception as exc:
        print("matrix load failed: %r" % (exc,), flush=True)
    finally:
        _matrix["loaded"] = True


def _find_entry(wl):
    for name, entry in _matrix["by_name"].items():
        if wl in name or name in wl:
            return entry
    return None


FAMILY_MATRIX = ("acgematrix3d", "acgematrix2d", "acgescale3d")


def _retrieve(text, limit=3):
    """抽词 -> 大小写不敏感子串匹配；PyXxx 未命中时试 AcXxx 别名。返回前 limit 个模块。

    几何变换族命中时额外带出 Ge.AcGePoint3d 名字含 transformBy 的 check：
    矩阵本身无 transformBy，点变换是 p.transformBy(matrix)。
    """
    _load_matrix()
    hits, seen = [], set()
    for w in re.findall(r"[A-Za-z][A-Za-z0-9_]{3,}", text):
        wl = w.lower()
        entry = _find_entry(wl)
        if entry is None and wl.startswith("py"):
            entry = _find_entry("ac" + wl[2:])
        if entry and entry["key"] not in seen:
            seen.add(entry["key"])
            hits.append(entry)
        if len(hits) >= limit:
            break
    family_hit = any(e["module"].lower() in FAMILY_MATRIX for e in hits)
    if family_hit and "acgepoint3d" in _matrix["by_name"]:
        pt = dict(_matrix["by_name"]["acgepoint3d"])
        pt["checks"] = [c for c in pt["checks"] if "transformby" in c["name"].lower()]
        if pt["checks"]:
            pt["summary"] = {"pass": sum(1 for c in pt["checks"] if c["ok"]),
                             "fail": sum(1 for c in pt["checks"] if not c["ok"])}
            hits.append(pt)
    return hits


def _compose_user(text, hits):
    if not hits:
        return text, []
    lines = ["[实测记录（以此为准，不要编造）]"]
    matched = []
    for e in hits:
        matched.append(e["key"])
        parts = []
        for c in e["checks"][:3]:
            parts.append("%s —— %s" % (c["name"], (c.get("detail") or "")[:200]))
        lines.append("%s：%s" % (e["key"], "；".join(parts)))
    return text + "\n" + "\n".join(lines), matched


def _grounded(hits):
    """抽取式直答：命中即把矩阵结论原样拼出，事实不经过生成模型。"""
    if not hits:
        return "", []
    lines = ["【实测结论（来自 module_matrix.json，非模型生成）】"]
    grounded_modules = []
    for e in hits:
        grounded_modules.append(e["key"])
        short = e["module"]
        for c in e["checks"]:
            lines.append("- %s：%s —— %s" % (short, c["name"], (c.get("detail") or "")[:320]))
        s = e.get("summary") or {}
        lines.append("参考：矩阵里 %s 共 %d 条断言，pass %s / fail %s"
                     % (short, len(e["checks"]), s.get("pass", "?"), s.get("fail", "?")))
    return "\n".join(lines), grounded_modules


_ctx_state = {"com_ms": None, "headless_ms": None, "src_last": None, "last_ts": "",
              "cache": None, "cache_ts": 0.0, "cache_key": "",
              "cache_hits": 0, "cache_total": 0, "ttl_ms": CTX_TTL_MS}
_guard_state = {"layer_inject": 0, "layer_drop": 0, "handle_block": 0,
                "stats_direct": 0, "erase_key_fix": 0}

DRAWING_TOOLS = {"cad_line", "cad_circle", "cad_arc", "cad_polyline", "cad_rect",
                 "cad_text", "cad_mtext", "cad_hatch", "cad_table", "cad_block_ref",
                 "cad_batch"}
_BATCH_CHILD_TOOLS = {"line", "circle", "arc", "polyline", "rect", "text", "mtext",
                      "hatch", "table", "block_ref"}
STATS_INTENT_RE = re.compile(r"(多少|几个|统计|数量|哪些层|分布)")


def _com_snapshot():
    """COM 正路：PowerShell 5.1 调 cad_com.ps1 stats，3 秒超时；失败返回 (None, ms)。"""
    t0 = time.time()
    cmd = [COM_PY, "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass",
           "-File", COM_PS1, "-Action", "stats"]
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=3.0)
        ms = round((time.time() - t0) * 1000)
        raw = (proc.stdout or "").strip()
        if not raw:
            return None, ms
        data = json.loads(raw.splitlines()[-1])
        if not data.get("ok"):
            return None, ms
        return {
            "doc": data.get("name"),
            "entity_total": int(data.get("count") or 0),
            "layers": {str(k): int(v) for k, v in (data.get("layers") or {}).items()},
            "types": {str(k): int(v) for k, v in (data.get("types") or {}).items()},
            "layer_names": [str(x) for x in (data.get("layerNames") or [])],
            "handles": [str(x) for x in (data.get("handles") or [])][:50],
            "handles_truncated": bool(data.get("handlesTruncated")),
        }, ms
    except Exception:
        return None, round((time.time() - t0) * 1000)


def _headless_snapshot():
    """COM 失败/超时后降级：accoreconsole 模板图只读统计（cad_ops.stats_script 口径）。"""
    t0 = time.time()
    try:
        sys.path.insert(0, CADMCP)
        import server as _mcp_server
        out = _mcp_server.call_headless({"py": HEADLESS_STATS_PY,
                                         "out": HEADLESS_STATS_OUT, "dwg": CTX_DWG,
                                         "timeout_sec": 60})
        ms = round((time.time() - t0) * 1000)
        res = out.get("python_out") if out.get("ok") else None
        if not isinstance(res, dict) or not res.get("ok"):
            return None, ms
        layers = {str(k): int(v) for k, v in (res.get("by_layer") or {}).items()}
        types = {str(k): int(v) for k, v in (res.get("by_class") or {}).items()}
        return {"doc": res.get("filename"),
                "entity_total": int(res.get("total") or 0), "layers": layers,
                "types": types, "layer_names": sorted(layers.keys()),
                "handles": [str(x) for x in (res.get("handles") or [])][:50],
                "handles_truncated": bool(res.get("handles_truncated"))}, ms
    except Exception:
        return None, round((time.time() - t0) * 1000)


def _fresh_mark(s):
    if not s.get("cached"):
        return "（刚刚）"
    sec = max(1, round((s.get("age_ms") or 0) / 1000))
    return "（%d 秒前）" % sec


def _render_prefix(s):
    fresh = _fresh_mark(s)
    if s["src"] == "none":
        return "[宿主快照 仅参照不影响执行 %s%s 取图失败 来源=none %dms]" % (s["ts"], fresh, s["ms"])
    layers = sorted((s["layers"] or {}).items(), key=lambda kv: (-kv[1], kv[0]))
    lstr, extra = [], 0
    for i, (n, c) in enumerate(layers):
        if i < 6:
            lstr.append("%s=%d" % (n, c))
        else:
            extra += c
    if extra:
        lstr.append("+%d" % extra)
    names = sorted(s["layer_names"] or [])
    nstr = names[:6]
    if len(names) > 6:
        nstr.append("+%d" % (len(names) - 6))
    query_ms = s.get("query_ms", s.get("ms") or 0)
    return "[宿主快照 仅参照不影响执行 截至%s%s 实体数=%d 图层分布=%s 图层名=%s 来源=%s %dms]" % (
        s["ts"], fresh, s["entity_total"] or 0, ",".join(lstr) or "-",
        ",".join(nstr) or "-", s["src"], query_ms)


def _ctx_snapshot(force=False):
    """前置现状查询，带 TTL 缓存（默认 45 秒，PYARX_CTX_TTL 覆盖）。

    缓存只存快照结果，不缓存模型回答；键取当前文档名（拿不到用单一 key），换图后新快照覆盖旧缓存。
    """
    now = time.time()
    cache = _ctx_state["cache"]
    if not force and cache:
        age = round((now - _ctx_state["cache_ts"]) * 1000)
        if age <= _ctx_state["ttl_ms"]:
            _ctx_state["cache_total"] += 1
            _ctx_state["cache_hits"] += 1
            snap = dict(cache)
            snap.update({"cached": True, "age_ms": age, "ms": 0,
                         "ttl_ms": _ctx_state["ttl_ms"]})
            snap["prefix"] = _render_prefix(snap)
            return snap
    _ctx_state["cache_total"] += 1

    ts = datetime.now().strftime("%H:%M")
    t0 = time.time()
    snap, com_ms = _com_snapshot()
    _ctx_state["com_ms"] = com_ms
    if snap is None:
        snap, hl_ms = _headless_snapshot()
        _ctx_state["headless_ms"] = hl_ms
        src = "headless" if snap is not None else "none"
    else:
        src = "com"
    if snap is None:
        snap = {"doc": None, "entity_total": None, "layers": {}, "types": {},
                "layer_names": [], "handles": [], "handles_truncated": True}
    total_ms = round((time.time() - t0) * 1000)
    snap.update({"ts": ts, "src": src, "ms": total_ms, "query_ms": total_ms,
                 "cached": False, "age_ms": 0, "ttl_ms": _ctx_state["ttl_ms"]})
    snap["prefix"] = _render_prefix(snap)
    _ctx_state["src_last"] = src
    _ctx_state["last_ts"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    _ctx_state["cache"] = dict(snap)
    _ctx_state["cache_ts"] = now
    _ctx_state["cache_key"] = snap.get("doc") or "default"
    return snap


def _ctx_log(text, snap):
    """每次调用落一行；try/finally 保证异常请求也有 src=none 记录。"""
    try:
        first = (text or "").replace("\n", " ")[:20]
        et = snap["entity_total"] if snap.get("entity_total") is not None else "-"
        line = "%s | %s | %s | %d | %s | %d | %d" % (
            _ctx_state["last_ts"], first, snap.get("src", "none"), snap.get("ms", 0),
            et, len(snap.get("layers") or {}), len(snap.get("layer_names") or []))
        with open(CTX_LOG, "a", encoding="utf-8") as f:
            f.write(line + "\n")
    except Exception:
        pass


def validate(tc):
    """校验 tool_call 合法性与参数类型。返回 (ok, reason)。"""
    if not isinstance(tc, dict):
        return False, "输出不是 JSON 对象"
    if "_parse_error" in tc:
        return False, "JSON 解析失败"
    name = tc.get("name")
    if name not in ALLOWED:
        return False, "未知工具：%r（只允许 17 个 cad_*）" % (name,)
    args = tc.get("arguments")
    if args is None:
        args = {}
    if not isinstance(args, dict):
        return False, "arguments 不是对象"
    schema = SCHEMAS.get(name) or {}
    isc = schema.get("inputSchema") or {}
    props = isc.get("properties") or {}
    for req in (isc.get("required") or []):
        if req not in args:
            return False, "缺必填参数：%s" % req
    for k, v in args.items():
        if k == "timeout_sec":
            continue
        prop = props.get(k)
        if not prop:
            return False, "未知参数：%s" % k
        want = prop.get("type")
        if want == "number" and not isinstance(v, (int, float)):
            return False, "参数 %s 应为数字，实际 %r" % (k, v)
        if want == "integer" and not isinstance(v, int):
            return False, "参数 %s 应为整数，实际 %r" % (k, v)
        if want == "string" and not isinstance(v, str):
            return False, "参数 %s 应为字符串，实际 %r" % (k, v)
        if want == "array" and not isinstance(v, list):
            return False, "参数 %s 应为数组，实际 %r" % (k, v)
    return True, "ok"


def _norm_handle(h):
    return str(h).strip().upper().lstrip("0")


def _erase_key_fix(tc):
    """cad_erase 的键名写成单数 handle 时，确定性改写成 handles 数组。

    1.5B 在 22 条 handles 样本、2 epoch 训练下仍会吐 {"handle": "698"}——
    因为 cad_offset 恰好用单数 handle，两个键在数据里并存，模型分不开。
    这是键名规范问题，交给宿主改比再训几轮更可靠。
    """
    repairs = []
    if not isinstance(tc, dict) or str(tc.get("name") or "") != "cad_erase":
        return tc, repairs
    args = tc.get("arguments")
    if not isinstance(args, dict) or "handles" in args:
        return tc, repairs
    hv = args.get("handle")
    if hv is None:
        return tc, repairs
    vals = hv if isinstance(hv, list) else [hv]
    del args["handle"]
    args["handles"] = [str(v) for v in vals]
    _guard_state["erase_key_fix"] = _guard_state.get("erase_key_fix", 0) + 1
    repairs.append({"type": "erase_key_fix", "from": "handle",
                    "to": "handles", "handles": args["handles"]})
    return tc, repairs


def _existing_layer_in_text(text, layer_names):
    t = (text or "")
    names = [str(x) for x in layer_names or []]
    for name in sorted((x for x in names if x != "0"), key=len, reverse=True):
        if re.search(r"(?<![A-Za-z0-9_])" + re.escape(name) + r"(?![A-Za-z0-9_])", t, re.I):
            return name
    if "0" in names and re.search(r"(?:层|layer)\s+0\b", t, re.I):
        return "0"
    return None


def _layer_guard(tc, text, snapshot):
    """绘图工具补全已存在图层；不存在且未要求新建时去掉 layer 字段。"""
    repairs = []
    if not isinstance(tc, dict):
        return tc, repairs
    name = str(tc.get("name") or "")
    args = tc.get("arguments")
    if not isinstance(args, dict):
        return tc, repairs
    existing = snapshot.get("layer_names") or []
    target = _existing_layer_in_text(text, existing)

    def _fix(tool, a):
        if not isinstance(a, dict):
            return
        layer = a.get("layer")
        if layer is not None and str(layer).lower() not in {str(x).lower() for x in existing}:
            # 用户明确点名的层即使当前不存在也要保留：cad_* 会按需建层，
            # 删掉参数等于违背指令。只有模型自己编的层才丢。
            if str(layer) in (text or ""):
                repairs.append({"type": "layer_keep", "tool": tool, "layer": str(layer)})
                return
            del a["layer"]
            repairs.append({"type": "layer_drop", "tool": tool, "layer": str(layer)})
            _guard_state["layer_drop"] += 1
        elif layer is None and target is not None and tool in _BATCH_CHILD_TOOLS:
            a["layer"] = target
            repairs.append({"type": "layer_inject", "tool": tool, "layer": target})
            _guard_state["layer_inject"] += 1

    if name in DRAWING_TOOLS:
        short = name[4:] if name.startswith("cad_") else name
        if name == "cad_batch":
            for op in args.get("ops") or []:
                if isinstance(op, dict):
                    _fix(str(op.get("tool") or ""), op.get("args") or {})
        else:
            _fix(short, args)
    return tc, repairs


def _handle_guard(tc, snapshot):
    """句柄不在现状清单时拦截写操作；快照截断时放行并标记 warning。"""
    warnings = []
    blocked = None
    auto_query = None
    if not isinstance(tc, dict):
        return tc, blocked, auto_query, warnings
    args = tc.get("arguments")
    if not isinstance(args, dict):
        return tc, blocked, auto_query, warnings
    handles = args.get("handles")
    if not isinstance(handles, list) or not handles:
        return tc, blocked, auto_query, warnings

    if snapshot.get("handles_truncated"):
        warnings.append({"type": "handle_scope",
                         "message": "句柄清单已截断，按现状清单无法完成越界校验"})
        return tc, blocked, auto_query, warnings

    existing = {_norm_handle(h) for h in snapshot.get("handles") or []}
    unknown = [str(h) for h in handles if _norm_handle(h) not in existing]
    if not unknown:
        return tc, blocked, auto_query, warnings

    _guard_state["handle_block"] += 1
    show_handles = ", ".join((snapshot.get("handles") or [])[:10])
    auto_query = {
        "tool": "cad_query", "arguments": {},
        "result": {"source": "current_snapshot",
                   "count": snapshot.get("entity_total"),
                   "handles": snapshot.get("handles") or [],
                   "layers": snapshot.get("layers") or {},
                   "types": snapshot.get("types") or {}},
    }
    blocked = {
        "blocked": "unknown_handle", "handles": unknown,
        "say": "图上无此句柄: %s（现状实体 %s，句柄: %s）"
               % (", ".join(unknown), snapshot.get("entity_total"), show_handles or "无"),
    }
    return None, blocked, auto_query, warnings


def _stats_direct(text, snapshot, tc):
    """统计意图直接以前置快照数字作答；模型 cad_stats 不作为事实来源。"""
    if not STATS_INTENT_RE.search(text or ""):
        return "", None, None
    if not snapshot or snapshot.get("src") == "none" or snapshot.get("entity_total") is None:
        return "", None, None
    _guard_state["stats_direct"] += 1
    layers = sorted((snapshot.get("layers") or {}).items(), key=lambda kv: (-kv[1], kv[0]))
    types = sorted((snapshot.get("types") or {}).items())
    lines = [
        "【现状直答（来自前置快照，非模型生成）】",
        "实体总数：%d" % int(snapshot["entity_total"]),
        "图层分布：%s" % ("，".join("%s=%d" % (n, c) for n, c in layers) or "-"),
        "图层：%s" % (",".join(sorted(snapshot.get("layer_names") or [])) or "-"),
        "类型：%s" % ("，".join("%s=%d" % (n, c) for n, c in types) or "-"),
    ]
    mismatch = None
    final_tc = None
    if isinstance(tc, dict) and tc.get("name") == "cad_stats":
        mismatch = "模型 cad_stats 未执行；以前置现状快照为准"
    return "\n".join(lines), final_tc, mismatch


def _enforce(text, tc, snapshot):
    tc = copy.deepcopy(tc) if isinstance(tc, dict) else tc
    repairs, warnings = [], []
    blocked, auto_query, stats_answer, mismatch = None, None, "", None
    final_tc = tc

    stats_answer, stats_tc, mismatch = _stats_direct(text, snapshot, tc)
    if stats_answer:
        final_tc = stats_tc
    else:
        final_tc, repairs = _layer_guard(tc, text, snapshot)
        final_tc, key_repairs = _erase_key_fix(final_tc)
        repairs += key_repairs
        final_tc, blocked, auto_query, warnings = _handle_guard(final_tc, snapshot)

    return {
        "final_tc": final_tc, "repairs": repairs, "warnings": warnings,
        "blocked": blocked, "auto_query": auto_query,
        "stats_answer": stats_answer, "mismatch": mismatch,
    }

_state = {"tok": None, "mdl": None, "lora": False, "loading": False,
          "err": "", "loaded_at": "", "load_sec": 0}


def _load(lora=True):
    if _state["loading"]:
        return
    _state["loading"] = True
    t0 = time.time()
    try:
        import torch
        torch.set_num_threads(4)
        from transformers import AutoModelForCausalLM, AutoTokenizer
        tok = AutoTokenizer.from_pretrained(BASE_MODEL)
        mdl = AutoModelForCausalLM.from_pretrained(BASE_MODEL)
        used_lora = False
        if lora and os.path.isdir(ADAPTER):
            try:
                from peft import PeftModel
                mdl = PeftModel.from_pretrained(mdl, ADAPTER)
                used_lora = True
            except Exception as exc:
                _state["err"] = "adapter 加载失败：%r" % (exc,)
        mdl.eval()
        _state.update({"tok": tok, "mdl": mdl, "lora": used_lora,
                       "loaded_at": time.strftime("%H:%M:%S"),
                       "load_sec": round(time.time() - t0, 1)})
    except Exception:
        _state["err"] = traceback.format_exc()[-600:]
    finally:
        _state["loading"] = False


def _reload():
    _state.update({"tok": None, "mdl": None, "lora": False})
    _load(True)


def chat(text, max_new=320, retrieval=True, snapshot=None):
    tok, mdl = _state["tok"], _state["mdl"]
    if tok is None or mdl is None:
        raise RuntimeError("模型未加载")
    import torch
    hits = _retrieve(text) if retrieval else []
    user_text, matched = _compose_user(text, hits)
    grounded, grounded_modules = _grounded(hits)
    sys_text = SYS
    if snapshot and snapshot.get("prefix") and os.environ.get("PYARX_CTX_INJECT", "0") == "1":
        # 默认不注入：1.5B 实测两条路都跑偏——放 user 里会复述快照并在空图时拒绝执行，
        # 放 system 里会先吐 cad_stats。前置查询照跑，改由宿主三道强制消费 snapshot：
        # layer_inject / handle_block / stats_direct，与这段文本是否进 prompt 无关。
        sys_text = SYS + "\n当前图纸快照（只作参照，不要复述）：" + snapshot["prefix"]
    msgs = [{"role": "system", "content": sys_text}, {"role": "user", "content": user_text}]
    prompt = tok.apply_chat_template(msgs, tokenize=False, add_generation_prompt=True)
    ids = tok(prompt, return_tensors="pt")
    t_m0 = time.time()
    with torch.no_grad():
        out = mdl.generate(**ids, max_new_tokens=int(max_new), do_sample=False,
                           pad_token_id=tok.eos_token_id)
    t_model_ms = round((time.time() - t_m0) * 1000)
    new_ids = out[0][ids["input_ids"].shape[1]:]
    raw = tok.decode(new_ids, skip_special_tokens=True).strip()
    blocks = TC_RE.findall(raw)
    extra_calls = max(0, len(blocks) - 1)
    tc = None
    m = TC_RE.search(raw)
    if m:
        try:
            tc = json.loads(m.group(1))
        except Exception:
            tc = {"_parse_error": m.group(1)[:200]}
    return raw, tc, matched, grounded, grounded_modules, t_model_ms, extra_calls


def _pick_train_log(override=None):
    logs_dir = os.path.join(ROOT, "logs")
    if override:
        p = os.path.join(logs_dir, os.path.basename(str(override)))
        if os.path.isfile(p):
            return p
    best = None
    if os.path.isdir(logs_dir):
        for f in os.listdir(logs_dir):
            if f.lower().startswith("train") and f.lower().endswith(".log"):
                p = os.path.join(logs_dir, f)
                if best is None or os.path.getmtime(p) > os.path.getmtime(best):
                    best = p
    return best


def train_status(override=None):
    st = {"points": [], "phase": "unknown", "rows": 0, "epochs": 0, "batch": 0,
          "total_steps": 0, "cur_step": 0, "cur_ep": 0, "last_loss": None,
          "adapter": os.path.isdir(ADAPTER), "adapter_files": 0, "log_age": None,
          "log_file": None, "parse": False, "stale": None}
    log_file = _pick_train_log(override)
    if log_file:
        st["log_file"] = os.path.basename(log_file)
        try:
            st["log_age"] = round(time.time() - os.path.getmtime(log_file), 1)
            with open(log_file, encoding="utf-8", errors="replace") as f:
                txt = f.read()
            tm = TRAIN_RE.search(txt)
            if tm:
                st["rows"], st["epochs"], st["batch"] = (int(x) for x in tm.groups())
                st["total_steps"] = -(-st["rows"] // max(1, st["batch"]) * st["epochs"])
            for a, b, c, d in LINE_RE.findall(txt):
                st["points"].append({"ep": int(a), "step": int(b),
                                     "loss": float(c), "sec": int(d)})
                st["cur_ep"], st["cur_step"], st["last_loss"] = int(a), int(b), float(c)
            if "[4/4]" in txt:
                st["phase"] = "done"
            elif "[3/4]" in txt:
                st["phase"] = "training"
            elif "[2/4]" in txt:
                st["phase"] = "loading"
            st["parse"] = True
            st["stale"] = bool(st["log_age"] > 300 and "done" not in txt)
        except Exception:
            st["parse"] = False
            st["stale"] = None
    if st["adapter"]:
        try:
            st["adapter_files"] = len(os.listdir(ADAPTER))
        except Exception:
            pass
    if os.path.isfile(DATA):
        try:
            with open(DATA, encoding="utf-8") as f:
                st["data_rows"] = sum(1 for line in f if line.strip())
        except Exception:
            st["data_rows"] = 0
    _load_matrix()
    st["matrix_modules"] = len(_matrix["modules"])
    st["ctx_com_ms"] = _ctx_state["com_ms"]
    st["ctx_headless_ms"] = _ctx_state["headless_ms"]
    st["ctx_src_last"] = _ctx_state["src_last"]
    st["ctx_last_ts"] = _ctx_state["last_ts"]
    st["ctx_note"] = "COM 正路未测（无 CAD 运行）"
    total = _ctx_state["cache_total"]
    st["ctx_cache_hit_rate"] = round(_ctx_state["cache_hits"] / total, 3) if total else 0.0
    st["ctx_ttl_ms"] = _ctx_state["ttl_ms"]
    st["lock_timeout_s"] = LOCK_TIMEOUT_S
    st["gen_timeout_s"] = GEN_TIMEOUT_S
    st["guard"] = dict(_guard_state)
    return st


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *args):
        pass

    def _send(self, code, body, ctype):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _json(self, obj, code=200):
        self._send(code, json.dumps(obj, ensure_ascii=False).encode("utf-8"),
                   "application/json; charset=utf-8")

    def _context_obj(self, snap):
        return {
            "ts": snap.get("ts"), "src": snap.get("src"), "ms": snap.get("ms"),
            "entity_total": snap.get("entity_total"), "layers": snap.get("layers"),
            "types": snap.get("types"), "layer_names": snap.get("layer_names"),
            "handles": snap.get("handles") or [],
            "handles_truncated": bool(snap.get("handles_truncated")),
            "prefix": snap.get("prefix"), "doc": snap.get("doc"),
            "cached": snap.get("cached"), "age_ms": snap.get("age_ms"),
            "query_ms": snap.get("query_ms"), "ttl_ms": snap.get("ttl_ms"),
        }

    def do_GET(self):
        try:
            parsed = urlparse(self.path)
            qs = parse_qs(parsed.query)
            if parsed.path.startswith("/api/status"):
                st = train_status()
                st["model"] = {"loading": _state["loading"],
                               "ready": _state["mdl"] is not None,
                               "lora": _state["lora"],
                               "loaded_at": _state["loaded_at"],
                               "load_sec": _state["load_sec"],
                               "err": _state["err"]}
                st["validate"] = dict(_strikes)
                st["tools_allowed"] = len(ALLOWED)
                return self._json(st)
            if parsed.path.startswith("/api/train-status"):
                return self._json(train_status(override=qs.get("file", [None])[0]))
            if parsed.path.startswith("/api/context"):
                cache = _ctx_state["cache"]
                if not cache:
                    return self._json({"available": False})
                age = round((time.time() - _ctx_state["cache_ts"]) * 1000)
                cached = age <= _ctx_state["ttl_ms"]
                obj = self._context_obj(cache)
                obj.update({"cached": cached, "age_ms": age,
                            "ms": 0 if cached else cache.get("query_ms") or 0})
                return self._json(obj)
            if parsed.path in ("/", "/index.html"):
                path = os.path.join(HERE, "index.html")
                with open(path, "rb") as f:
                    return self._send(200, f.read(), "text/html; charset=utf-8")
            self._send(404, b"not found", "text/plain")
        except Exception as exc:
            self._json({"ok": False, "error": repr(exc)[:300]}, 500)

    def do_POST(self):
        try:
            n = int(self.headers.get("Content-Length") or 0)
            try:
                payload = json.loads(self.rfile.read(n).decode("utf-8") or "{}") if n else {}
            except Exception:
                payload = {}

            if self.path.startswith("/api/context/refresh"):
                snap = _ctx_snapshot(force=True)
                return self._json({"ok": True, "context": self._context_obj(snap)})

            if self.path.startswith("/api/reload"):
                threading.Thread(target=_reload, daemon=True).start()
                return self._json({"ok": True, "loading": True})

            if not self.path.startswith("/api/chat"):
                return self._send(404, b"not found", "text/plain")

            if _state["mdl"] is None:
                if not _state["loading"]:
                    threading.Thread(target=_load, daemon=True).start()
                return self._json({"ok": False, "err": _state["err"] or "模型正在加载，几秒后再试"}, 503)

            t0 = time.time()
            text = payload.get("text") or ""
            snapshot = None
            if not _chat_lock.acquire(timeout=LOCK_TIMEOUT_S):
                return self._json({"ok": False, "error": "busy",
                                   "message": "模型正在处理上一个请求"}, 503)
            try:
                snapshot = _ctx_snapshot()
                retrieval = payload.get("retrieval", True)
                worker = {}

                def _run():
                    try:
                        (worker["raw"], worker["tc"], worker["matched"],
                         worker["grounded"], worker["grounded_modules"],
                         worker["t_model_ms"], worker["extra_calls"]) = chat(
                            text, payload.get("max_new") or 320, retrieval, snapshot)
                    except Exception as exc:
                        worker["error"] = exc

                th = threading.Thread(target=_run, daemon=True)
                th.start()
                th.join(GEN_TIMEOUT_S)
                if th.is_alive():
                    return self._json({"ok": False, "error": "timeout",
                                       "hint": "模型未在 %d 秒内返回" % int(GEN_TIMEOUT_S)}, 504)
                if "error" in worker:
                    return self._json({"ok": False, "err": repr(worker["error"])[:300]}, 500)
            finally:
                _ctx_log(text, snapshot or {"src": "none", "ms": 0, "entity_total": None,
                                            "layers": {}, "layer_names": []})
                _chat_lock.release()

            raw, tc = worker["raw"], worker["tc"]
            fake = _fake_tools(raw)
            enf = _enforce(text, tc, snapshot)
            final_tc = enf["final_tc"]
            if final_tc is None:
                if enf["blocked"]:
                    vok, vreason = True, "宿主拦截：" + enf["blocked"]["say"]
                elif enf["stats_answer"]:
                    vok, vreason = True, "统计类宿主直答"
                elif fake:
                    vok, vreason = False, "纯文本里编造了不存在的工具：" + ",".join(fake)
                    _strikes["rejected"] += 1
                else:
                    vok, vreason = True, "无 tool_call（纯文本回答）"
            else:
                vok, vreason = validate(final_tc)
                _strikes["passed" if vok else "rejected"] += 1
            return self._json({"ok": True, "raw": raw,
                               "model_tool_call": tc, "tool_call": final_tc,
                               "script": _script_for_tc(final_tc),
                               "repairs": enf["repairs"],
                               "blocked": enf["blocked"],
                               "auto_query": enf["auto_query"],
                               "warnings": enf["warnings"],
                               "stats_answer": enf["stats_answer"],
                               "mismatch": enf["mismatch"],
                               "fake_tools": fake,
                               "extra_calls": worker["extra_calls"],
                               "matched": worker["matched"], "retrieval": bool(retrieval),
                               "grounded": worker["grounded"],
                               "grounded_modules": worker["grounded_modules"],
                               "valid": vok, "reason": vreason,
                               "context": self._context_obj(snapshot),
                               "t_ctx_ms": snapshot.get("ms"),
                               "t_model_ms": worker["t_model_ms"],
                               "t_total_ms": round((time.time() - t0) * 1000),
                               "seconds": round(time.time() - t0, 2),
                               "model": "base+lora" if _state["lora"] else "base"})
        except Exception as exc:
            return self._json({"ok": False, "error": repr(exc)[:300]}, 500)


if __name__ == "__main__":
    threading.Thread(target=_load, daemon=True).start()
    srv = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print("PyArx playground -> http://127.0.0.1:%d" % PORT, flush=True)
    srv.serve_forever()
