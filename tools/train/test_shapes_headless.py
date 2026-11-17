# -*- coding: utf-8 -*-
"""test_shapes_headless.py —— 把 gen_shapes 的 30 个模板真的送进 CAD 跑一遍。

静态测试（test_shapes.py）只证明"话和参数对得上"，证明不了"工具层真能执行"。
这里取每个模板 1 条样本，抽出应当成功的调用，交给 cad_ops.build_script()
生成内层脚本，在 accoreconsole 里执行，看 OUT 的 ok/count/errors。

判定：
  造图类   ok=True 且 count>0（layer 单独不作为判据）
  只读类   ok=True
  自纠类   只验证"修正后"的调用

用法：python test_shapes_headless.py
"""
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TRAIN = r"G:\PyArx\tools\train"
CADMCP = r"G:\PyArx\tools\cadmcp"
TMP = r"C:\temp"
CONSOLE = r"G:\PyArx\ACAD\accoreconsole.exe"
ENGINE = r"G:\PyArx\X64\Release\PyArxEngine.crx"
TEMPLATE = r"C:\temp\pyarx_db_save_test.dwg"
OUT_JSON = os.path.join(TMP, "shape_result.json")
WORK = os.path.join(TMP, "shape_probe.dwg")

sys.path.insert(0, TRAIN)
sys.path.insert(0, CADMCP)
import gen_shapes as S  # noqa: E402
import cad_ops  # noqa: E402

BUILD_KINDS = {"line", "circle", "arc", "text", "mtext", "polyline", "hatch",
               "table", "rect", "block_ref", "layer"}
READ_KINDS = {"query", "stats"}
OTHER_KINDS = {"offset", "erase", "save"}


def _strip(name):
    return name[4:] if name.startswith("cad_") else name


def _calls_of(sample, kind):
    """抽该样本里"应当成功"的调用序列。"""
    res = []
    msgs = sample["messages"]
    if kind == "repair":
        last = [m for m in msgs if m["role"] == "assistant"][-1]
        for c in (last.get("tool_calls") or []):
            res.append((c["function"]["name"], json.loads(c["function"]["arguments"])))
    else:
        for m in msgs:
            for c in (m.get("tool_calls") or []):
                res.append((c["function"]["name"], json.loads(c["function"]["arguments"])))
    return res


def _expand_rect(args, draw):
    """与 cad_ops.script_for 的 rect 分支同一口径：四角 polyline + 等分线。"""
    p1 = args.get("p1") or [0, 0]
    p2 = args.get("p2") or [0, 0]
    x1, y1 = float(p1[0]), float(p1[1])
    x2, y2 = float(p2[0]), float(p2[1])
    lx, ly, rx, ry = min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2)
    sub = {"pts": [[lx, ly], [rx, ly], [rx, ry], [lx, ry]], "closed": True}
    for k in ("layer", "layer_rgb", "color", "timeout_sec"):
        if args.get(k) is not None:
            sub[k] = args[k]
    draw.append({"tool": "polyline", "args": sub})
    draw.extend(cad_ops._expand_layout(lx, ly, rx, ry, args))


def _segments(calls):
    """造图类合并成一个脚本；只读/其它各自成脚本。"""
    draw = []
    segs = []
    for name, args in calls:
        n = _strip(name)
        if n == "rect":
            _expand_rect(args, draw)
        elif n in BUILD_KINDS:
            draw.append({"tool": n, "args": args})
        elif n in READ_KINDS:
            segs.append(("code", n, cad_ops.script_for(n, args)))
        else:
            segs.append(("code", n, cad_ops.script_for(n, args)))
    if draw:
        segs.insert(0, ("build", "draw:" + "+".join(d["tool"] for d in draw),
                        cad_ops.build_script(draw)))
    return segs


def main():
    if not os.path.isfile(TEMPLATE):
        raise SystemExit("模板图不存在：" + TEMPLATE)
    only = [a for a in sys.argv[1:] if not a.startswith("-")]

    cases = {}
    for kind, tpls in (("multi_call", S.MULTI_CALL_TPLS),
                       ("multi_turn", S.MULTI_TURN_TPLS),
                       ("repair", S.REPAIR_TPLS)):
        for fn in tpls:
            if only and only[0] != "all" and fn.__name__ not in only:
                continue
            if kind == "multi_call":
                user, calls, label = fn()
                sample = S._row([{"role": "system", "content": S.SYS_SHORT},
                                 {"role": "user", "content": user}, S._asst(*calls)], kind, label)
            elif kind == "multi_turn":
                user, steps, final, label = fn()
                msgs = [{"role": "system", "content": S.SYS_SHORT},
                        {"role": "user", "content": user}]
                for a, t in steps:
                    msgs += [a, t]
                msgs.append(final)
                sample = S._row(msgs, kind, label)
            else:
                user, bad, bad_res, good, label = fn()
                sample = S._row([{"role": "system", "content": S.SYS_SHORT},
                                 {"role": "user", "content": user},
                                 S._asst(*bad), S._tool(bad_res), S._asst(*good)], kind, label)
            cases[fn.__name__] = {"kind": kind, "user": user,
                                  "segs": _segments(_calls_of(sample, kind))}

    inner = ["import json, traceback",
             "CASES = " + repr({k: [{"what": s[1], "src": s[2]} for s in v["segs"]]
                                for k, v in cases.items()}),
             "RES = {}",
             "OUTP = r'%s'" % OUT_JSON.replace("\\", "/"),
             "def _flush():",
             "    with open(OUTP, 'w', encoding='utf-8') as f:",
             "        json.dump(RES, f, ensure_ascii=False, indent=1)",
             "_flush()",
             "for name, segs in CASES.items():",
             "    det = []",
             "    for seg in segs:",
             "        ns = {}",
             "        try:",
             "            exec(compile(seg['src'], '<seg>', 'exec'), ns)",
             "            out = ns.get('OUT') or {}",
             "            det.append({'what': seg['what'], 'ok': bool(out.get('ok')),",
             "                        'count': out.get('count'), 'errors': out.get('errors'),",
             "                        'warnings': out.get('warnings'),",
             "                        'handles': out.get('handles')})",
             "        except Exception as ex:",
             "            det.append({'what': seg['what'], 'ok': False,",
             "                        'err': repr(ex)[:260],",
             "                        'tb': traceback.format_exc()[-400:]})",
             "        RES[name] = det",
             "        _flush()",
             "    RES[name] = det",
             "_flush()",
             "RES['__done__'] = True",
             "_flush()"]
    body = "\n".join(inner)

    shutil.copyfile(TEMPLATE, WORK)
    py = os.path.join(TMP, "shape_inner.py")
    scr = os.path.join(TMP, "shape_inner.scr")
    with open(py, "w", encoding="utf-8") as f:
        f.write(body)
    with open(scr, "w", encoding="utf-8") as f:
        f.write('(setvar "SECURELOAD" 0)\n')
        f.write('(arxload "%s")\n' % ENGINE.replace("\\", "/"))
        f.write('(command "PYRUNFILE" "%s")\n' % py.replace("\\", "/"))
    if os.path.exists(OUT_JSON):
        os.remove(OUT_JSON)
    proc = subprocess.run([CONSOLE, "/i", WORK, "/s", scr], capture_output=True, timeout=600)
    if not os.path.exists(OUT_JSON):
        print("内层脚本没写出结果 exit=%s" % proc.returncode)
        print(proc.stdout.decode("utf-8", "ignore")[-1000:])
        return 1

    with open(OUT_JSON, encoding="utf-8") as f:
        res = json.load(f)

    bad = 0
    done = res.pop("__done__", False)
    for name, det in res.items():
        fails = [d for d in det if not d.get("ok")]
        tag = "" if not fails else "  <-- FAIL"
        if fails:
            bad += 1
        print("%-30s %s%s" % (name, "ok" if not fails else "失败 %d 段" % len(fails), tag))
        for d in fails:
            print("      %s: %s" % (d.get("what"), d.get("err") or json.dumps(
                {"errors": d.get("errors"), "warnings": d.get("warnings"),
                 "count": d.get("count")}, ensure_ascii=False)[:300]))
    print("\n模板 %d 个，实跑通过 %d，失败 %d%s"
          % (len(res), len(res) - bad, bad, "" if done else "（中途崩溃，未跑完）"))
    return 0 if bad == 0 and done else 1


if __name__ == "__main__":
    raise SystemExit(main())
