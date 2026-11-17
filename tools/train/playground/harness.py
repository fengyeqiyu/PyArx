# -*- coding: utf-8 -*-
"""PyArx 本地 harness：模型 → 工具执行 → 结果回灌 → 继续。

它不是对话框。每条 cad_* 调用都会真的下发到 CAD（COM bridge）或无头引擎
（accoreconsole），把结构化结果 {ok,count,handles,errors} 回灌给模型，
让模型决定继续、换工具还是自纠——只有这个闭环跑通，"会用 PyArxCore" 才算被验证。

通道：
  com      —— 走运行中的 CAD，快（百毫秒级），需要开着 CAD
  headless —— accoreconsole + PyArxEngine.crx，一轮约 8 秒，不碰用户 CAD
  auto     —— 先 com，CAD 没开就降级 headless

端口：8770（PYARX_HARNESS_PORT 可覆盖）
"""
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import threading
import time
import traceback
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

HERE = os.path.dirname(os.path.abspath(__file__))
CADMCP = r"G:\PyArx\tools\cadmcp"
TMP = r"C:\temp"
DWG_DEFAULT = r"C:\temp\pyarx_db_save_test.dwg"
# 持久化会话图纸：同一张图连续累积，每步 saveAs 回写
WORK_DWG = os.environ.get("PYARX_HARNESS_DWG", r"C:\temp\harness_work.dwg")
CONSOLE = os.environ.get("PYARX_CONSOLE", r"G:\PyArx\ACAD\accoreconsole.exe")

spec = importlib.util.spec_from_file_location("pg_app", os.path.join(HERE, "app.py"))
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)

if CADMCP not in sys.path:
    sys.path.insert(0, CADMCP)
import server as mcp

# 确定性路由优先：能确定的句式直接产出 cad_* 调用，不进模型（0 延迟、0 误差）；
# 规则未命中才落到 1.5B 兜底。PYARX_RULE_ROUTER=0 可整体关掉。
if HERE not in sys.path:
    sys.path.insert(0, HERE)
_RULE_ON = os.environ.get("PYARX_RULE_ROUTER", "1") != "0"
try:
    from rule_router import route as rule_route
except Exception:
    _RULE_ON = False
    rule_route = None

PORT = int(os.environ.get("PYARX_HARNESS_PORT", "8770"))
MAX_TURNS = int(os.environ.get("PYARX_HARNESS_TURNS", "4"))
GEN_LOCK = threading.Lock()
RUN_LOCK = threading.Lock()

_T = {"runs": 0, "ok": 0, "fail": 0, "last": None}
# 原话里出现这些词，说明还有后续动作，cad_batch 成功后不提前收尾
_CONT = ("然后", "再", "接着", "之后", "并且", "同时", "顺便", "还要")


# ---------------------------------------------------------------- prompt 拼装

def render(msgs):
    """与 train_lora_cpu.render 同口径拼 prompt。

    多轮推理必须和训练用同一格式：训练时 tool 结果就是 <|im_start|>tool 段，
    推理如果改用 apply_chat_template，模型会看到一个训练里没见过的模板。
    """
    parts = []
    for m in msgs:
        role = m.get("role")
        if role == "assistant" and m.get("tool_calls"):
            fn = m["tool_calls"][0]["function"]
            body = ('<tool_call>\n{"name": "%s", "arguments": %s}\n</tool_call>'
                    % (fn["name"], fn["arguments"]))
            parts.append("<|im_start|>assistant\n" + body + "<|im_end|>\n")
        else:
            parts.append("<|im_start|>%s\n%s<|im_end|>\n" % (role, m.get("content") or ""))
    parts.append("<|im_start|>assistant\n")
    return "".join(parts)


def gen(msgs, max_new=320):
    tok, mdl = app._state["tok"], app._state["mdl"]
    if tok is None or mdl is None:
        raise RuntimeError("模型未加载")
    import torch
    prompt = render(msgs)
    ids = tok(prompt, return_tensors="pt", truncation=True, max_length=1800)
    t0 = time.time()
    with GEN_LOCK:
        with torch.no_grad():
            out = mdl.generate(**ids, max_new_tokens=int(max_new), do_sample=False,
                               pad_token_id=tok.eos_token_id)
    new_ids = out[0][ids["input_ids"].shape[1]:]
    raw = tok.decode(new_ids, skip_special_tokens=True).strip()
    tc = None
    m = app.TC_RE.search(raw)
    if m:
        try:
            tc = json.loads(m.group(1))
        except Exception:
            tc = {"_parse_error": m.group(1)[:200]}
    return raw, tc, round((time.time() - t0) * 1000)


# ---------------------------------------------------------------- 执行

def _headless_code(name, args):
    ops = app._ops
    if name == "cad_stats":
        return ops.stats_script(args.get("max_scan", 5000))
    if name == "cad_query":
        return ops.query_script(args.get("cls", ""), args.get("layer", ""),
                                args.get("limit", 200))
    if name == "cad_erase":
        return ops.erase_script(args.get("handles"), args.get("layer", ""),
                                bool(args.get("all")), args.get("confirm", ""))
    return ops.script_for(name, args)


def reset_work(dwg=None):
    """重建会话图纸：清空回模板，下一轮从 0 实体开始。"""
    target = dwg or WORK_DWG
    try:
        if os.path.isfile(target):
            os.remove(target)
    except Exception:
        pass
    shutil.copyfile(DWG_DEFAULT, target)
    return {"ok": True, "dwg": target, "entities": 0}


def exec_headless(name, args, dwg=None):
    """持久化无头执行：在固定 work.dwg 上累积，每步末尾 saveAs 回写。

    关键差异（实测）：accoreconsole 里 DB.saveAs 是安全的，之前那个
    acad.exe 0x6f414 崩溃来自带 SecurityParams 的重载，无头分支不触发。
    没有这一步，每次调用都是全新副本，多轮闭环里模型看不到自己上一步画的东西。
    """
    work = dwg or WORK_DWG
    if not os.path.isfile(work):
        shutil.copyfile(DWG_DEFAULT, work)
    code = _headless_code(name, args)
    stem = os.path.join(TMP, "hh_%d_%d" % (int(time.time() * 1000),
                                           threading.get_ident() % 997))
    py, outj, scr = stem + ".py", stem + "_out.json", stem + ".scr"
    wrapped = (
        "import PyArxCore as P\n" + code +
        "\nSAVE = {'ok': False, 'err': ''}\n"
        "try:\n"
        "    DB.saveAs(r'%s')\n"
        "    SAVE['ok'] = True\n"
        "except Exception as _e:\n"
        "    SAVE['err'] = repr(_e)\n"
        "result['save'] = SAVE\n"
        "with open(r'%s', 'w', encoding='utf-8') as _f:\n"
        "    json.dump(result, _f, ensure_ascii=False)\n" % (work, outj)
    )
    with open(py, "w", encoding="utf-8") as f:
        f.write(wrapped)
    with open(scr, "w", encoding="utf-8") as f:
        f.write('(setvar "SECURELOAD" 0)\n')
        f.write('(arxload "G:/PyArx/X64/Release/PyArxEngine.crx")\n')
        f.write('(command "PYRUNFILE" "%s")\n' % py.replace("\\", "/"))
    if os.path.exists(outj):
        os.remove(outj)
    try:
        proc = subprocess.run([CONSOLE, "/i", work, "/s", scr],
                              capture_output=True, timeout=120)
    except Exception as exc:
        return {"ok": False, "error": repr(exc)[:200], "channel": "headless"}
    if not os.path.exists(outj):
        return {"ok": False, "error": "脚本没有写出结果 JSON",
                "channel": "headless", "exit": proc.returncode}
    try:
        with open(outj, encoding="utf-8") as f:
            res = json.load(f)
    except Exception as exc:
        return {"ok": False, "error": "读结果失败: %r" % (exc,), "channel": "headless"}
    if not isinstance(res, dict):
        return {"ok": False, "error": "结果不是对象", "channel": "headless"}
    res["channel"] = "headless"
    return res


def exec_tool(name, args, backend="auto", dwg=None):
    t0 = time.time()
    if backend in ("auto", "com"):
        r = mcp.call_cad(name, args)
        if r.get("ok") or backend == "com":
            r["channel"] = "com"
            r["ms"] = round((time.time() - t0) * 1000)
            return r
        if backend == "auto":
            r2 = exec_headless(name, args, dwg)
            r2["fallback_from"] = str(r.get("error") or "")[:160]
            r2["ms"] = round((time.time() - t0) * 1000)
            return r2
        r["channel"] = "com"
        r["ms"] = round((time.time() - t0) * 1000)
        return r
    r = exec_headless(name, args, dwg)
    r["ms"] = round((time.time() - t0) * 1000)
    return r


# ---------------------------------------------------------------- 闭环

def _as_msg(tc):
    return {"role": "assistant", "content": None,
            "tool_calls": [{"type": "function",
                            "function": {"name": str(tc.get("name") or ""),
                                         "arguments": json.dumps(tc.get("arguments") or {},
                                                                 ensure_ascii=False)}}]}


def agent(text, max_turns=MAX_TURNS, backend="auto", max_new=320, dwg=None):
    """跑一轮对话闭环，返回逐轮 trace。"""
    snap = app._ctx_snapshot()
    msgs = [{"role": "system", "content": app.SYS},
            {"role": "user", "content": text}]
    trace, t_start = [], time.time()
    _last_sig = None

    rule_call = None
    if _RULE_ON and rule_route:
        try:
            rule_call = rule_route(text)
        except Exception:
            rule_call = None

    for turn in range(1, int(max_turns) + 1):
        if turn == 1 and rule_call:
            # 规则命中：跳过模型，直接拿确定性调用
            raw, tc, gms, src = "", {"name": rule_call["tool"],
                                     "arguments": dict(rule_call.get("args") or {})}, 0, "rule"
        else:
            raw, tc, gms = gen(msgs, max_new)
            src = "model"
        enf = app._enforce(text, tc, snap)
        # 规则命中的 cad_stats 不走"快照直答"：快照可能已过期（实测回 0 实体、
        # 与图面不符），规则调用是确定性的，直接真执行才准。
        if src == "rule" and (tc or {}).get("name") == "cad_stats":
            enf = {"final_tc": tc, "repairs": [], "warnings": [],
                   "blocked": None, "stats_answer": None}
        # 规则命中的 cad_erase 同样绕过"句柄存在性"拦截：那个判断依赖宿主快照，
        # 而快照会滞后于真实图面（实测快照 0 实体、图上 4 实体，真句柄被误拦）。
        # 句柄是用户原话里给的，直接下发让 CAD 自己判存在更可靠。
        if src == "rule" and (tc or {}).get("name") == "cad_erase":
            enf = {"final_tc": tc, "repairs": [], "warnings": [],
                   "blocked": None, "stats_answer": None}
        f_tc = enf["final_tc"]
        rec = {"turn": turn, "raw": raw, "model_tool_call": tc,
               "tool_call": f_tc, "repairs": enf["repairs"],
               "warnings": enf["warnings"], "gen_ms": gms, "source": src}

        # 模型给纯文本 = 它认为做完了
        if f_tc is None:
            rec["done"] = "text"
            rec["blocked"] = enf["blocked"]
            rec["stats_answer"] = enf["stats_answer"]
            trace.append(rec)
            break

        # 宿主拦下（越界句柄等）：不执行，闭环到此为止
        if enf["blocked"]:
            rec["done"] = "blocked"
            rec["blocked"] = enf["blocked"]
            trace.append(rec)
            break

        # 模型输出缺工具名（如直接吐了 {"ops": [...]} 裸参数）：拦下并留证据，
        # 不要让 KeyError 冒成 HTTP 500（原来这条会把整条用例变成 ERR）。
        if not isinstance(f_tc, dict) or not str(f_tc.get("name") or "").strip():
            rec["done"] = "bad_call"
            rec["note"] = "模型输出缺少工具名，已拦截不送 CAD"
            trace.append(rec)
            break

        # 重复调用保护：拿到工具结果后模型常把同一条调用再吐一遍，
        # 真执行就会重复画图（实测复合指令连画 3 遍 after 5→10→15）。
        # 同样的 name+args 第二次出现即终止，不送 CAD。
        sig = f_tc["name"] + "|" + json.dumps(f_tc["arguments"], ensure_ascii=False,
                                             sort_keys=True)
        if sig == _last_sig:
            rec["done"] = "duplicate"
            rec["note"] = "与上一轮完全相同的调用，已拦截不重复执行"
            trace.append(rec)
            break
        _last_sig = sig

        vok, vreason = app.validate(f_tc)
        rec["valid"], rec["reason"] = vok, vreason
        if not vok:
            # 参数不合法就不送 CAD：把校验错误当成工具返回，让模型自己改
            trace.append(rec)
            msgs.append(_as_msg(f_tc))
            msgs.append({"role": "tool", "content": json.dumps(
                {"ok": False, "errors": [{"error": vreason}]}, ensure_ascii=False)})
            continue

        res = exec_tool(f_tc["name"], f_tc["arguments"], backend, dwg)
        rec["result"] = res
        trace.append(rec)
        msgs.append(_as_msg(f_tc))
        msgs.append({"role": "tool",
                     "content": json.dumps(res, ensure_ascii=False)[:2500]})

        # 规则命中 + 执行成功 + 原话没有后续动作要求 → 直接收尾，
        # 不再花 80 秒让模型复述同一条调用。
        if src == "rule" and res.get("ok") is True and not any(k in text for k in _CONT):
            rec["done"] = "rule"
            break

        # 复合指令一次画完就收尾：cad_batch 成功且原话没有「然后/再/接着」这类
        # 后续要求时，不必再花 80+ 秒让模型复述同一条调用（实测那 88 秒纯空转）。
        if (str(f_tc.get("name")) == "cad_batch" and res.get("ok") is True
                and not any(k in text for k in _CONT)):
            rec["done"] = "batch"
            break

    _T["runs"] += 1
    last_ok = all((t.get("result") or {}).get("ok", True) for t in trace if t.get("result"))
    _T["ok" if last_ok else "fail"] += 1
    _T["last"] = time.strftime("%H:%M:%S")

    return {"ok": True, "trace": trace, "turns": len(trace),
            "seconds": round(time.time() - t_start, 2), "backend": backend,
            "rule_hit": bool(rule_call),
            "rule_call": rule_call,
            "context": {"prefix": snap.get("prefix"), "src": snap.get("src"),
                        "entity_total": snap.get("entity_total"),
                        "handles": (snap.get("handles") or [])[:20]}}


def demo_steps(backend="headless"):
    """不经过模型的三步演示：画线 → 画圆 → 查统计，用来展示执行回包与状态累积。"""
    reset_work()
    steps = [("cad_line", {"p1": [0, 0], "p2": [100, 0]}),
             ("cad_circle", {"center": [50, 50], "radius": 10}),
             ("cad_stats", {})]
    trace = []
    for i, (name, args) in enumerate(steps, 1):
        t0 = time.time()
        if backend == "headless":
            res = exec_headless(name, args)
        else:
            res = exec_tool(name, args, backend)
        trace.append({"turn": i, "valid": True,
                      "tool_call": {"name": name, "arguments": args},
                      "result": res, "ms": round((time.time() - t0) * 1000)})
    return {"ok": True, "trace": trace, "turns": len(trace),
            "backend": backend, "demo": True, "seconds": None}


# ---------------------------------------------------------------- HTTP

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def _json(self, obj, code=200):
        body = json.dumps(obj, ensure_ascii=False, default=str).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _html(self):
        p = os.path.join(HERE, "harness.html")
        with open(p, encoding="utf-8") as f:
            body = f.read().encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        try:
            u = urlparse(self.path)
            if u.path in ("/", "/harness.html"):
                return self._html()
            if u.path == "/api/status":
                st = app._state
                return self._json({"ok": True, "ready": bool(st.get("mdl")),
                                   "lora": st.get("lora"),
                                   "load_sec": st.get("load_sec"),
                                   "err": (st.get("err") or "")[-300:],
                                   "model": os.path.basename(app.ADAPTER),
                                   "turns_max": MAX_TURNS,
                                   "stats": _T,
                                   "guard": app._guard_state})
            if u.path == "/api/context":
                return self._json({"ok": True, "context": app._ctx_snapshot()})
            return self._json({"ok": False, "error": "not found"}, 404)
        except Exception as exc:
            return self._json({"ok": False, "error": repr(exc)[:300]}, 500)

    def do_POST(self):
        try:
            n = int(self.headers.get("Content-Length") or 0)
            payload = json.loads(self.rfile.read(n).decode("utf-8") or "{}") if n else {}
            u = urlparse(self.path)
            if u.path == "/api/context/refresh":
                return self._json({"ok": True, "context": app._ctx_snapshot(force=True)})
            if u.path == "/api/reset":
                return self._json(reset_work(payload.get("dwg") or None))
            if u.path == "/api/demo":
                if not RUN_LOCK.acquire(timeout=180):
                    return self._json({"ok": False, "error": "busy"}, 503)
                try:
                    return self._json(demo_steps(payload.get("backend") or "headless"))
                finally:
                    RUN_LOCK.release()
            if u.path != "/api/agent":
                return self._json({"ok": False, "error": "not found"}, 404)

            text = (payload.get("text") or "").strip()
            if not text:
                return self._json({"ok": False, "error": "empty text"}, 400)
            if app._state.get("mdl") is None:
                # 规则命中的指令不经过模型，所以无需权重；只有要模型兜底时才拒绝
                hit = None
                if _RULE_ON and rule_route:
                    try:
                        hit = rule_route(text)
                    except Exception:
                        hit = None
                if not hit:
                    return self._json({"ok": False, "error": "模型未加载"}, 503)
            if not RUN_LOCK.acquire(timeout=180):
                return self._json({"ok": False, "error": "busy"}, 503)

            try:
                out = agent(text,
                            max_turns=payload.get("max_turns") or MAX_TURNS,
                            backend=(payload.get("backend") or "auto"),
                            max_new=payload.get("max_new") or 320,
                            dwg=payload.get("dwg") or None)
            finally:
                RUN_LOCK.release()
            return self._json(out)
        except Exception as exc:
            return self._json({"ok": False, "error": repr(exc)[:400],
                               "tb": traceback.format_exc()[-600:]}, 500)


if __name__ == "__main__":
    app.BASE_MODEL = os.environ.get("CAD_BASE_MODEL", r"D:\models\qwen2.5-1.5b")
    _new = os.path.join(app.ROOT, "out", "qwen1.5b-cad-lora-255row")
    _prev = os.path.join(app.ROOT, "out", "qwen1.5b-cad-lora-248row")
    _eps = os.path.join(app.ROOT, "out", "qwen1.5b-cad-lora-3ep")
    _base = os.path.join(app.ROOT, "out", "qwen1.5b-cad-lora")
    _env = os.environ.get("CAD_ADAPTER_OUT", "").strip()
    if _env and os.path.isdir(_env):
        app.ADAPTER = _env
    elif os.path.isdir(_new):
        app.ADAPTER = _new
    elif os.path.isdir(_prev):
        app.ADAPTER = _prev
    elif os.path.isdir(_eps):
        app.ADAPTER = _eps
    else:
        app.ADAPTER = _base
    app.PORT = PORT
    print("harness adapter:", app.ADAPTER, flush=True)
    # 训练占着内存时用 NOMODEL=1 起界面：只服务静态页与现状查询，不加载权重
    if os.environ.get("PYARX_HARNESS_NOMODEL", "0") != "1":
        app._load(lora=True)
    else:
        app._state["err"] = "NOMODEL=1：界面预览模式，未加载权重"
    srv = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print("PyArx harness -> http://127.0.0.1:%d" % PORT, flush=True)
    srv.serve_forever()
