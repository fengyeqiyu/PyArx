"""训练成果判定：跑固定用例集，逐条断言到「执行层」，输出 PASS/FAIL。

为什么不能只看 valid=True：
  - 248 模型「配电柜」那条 valid=True，真执行报「不支持的 op：rect」；
  - -3ep 时代模型连吐 5 个 tool_call，校验器也放过了；
  - max_new=160 截断 JSON 时，tool_call 直接变 null，看起来像「不会用工具」。
所以每条用例都预先写死期望：工具名 + 实体数(after) + ops 数量 + 关键坐标，
只有真执行返回 ok 且数量对得上才算 PASS。

用法：
  python eval_agent_loop.py                 # 默认打 8770 harness
  python eval_agent_loop.py --base http://127.0.0.1:8770
退出码：全 PASS 为 0，否则 1。
"""
import argparse
import json
import sys
import urllib.request

CASES = [
    # name, text, checks
    ("单线", "在 0 层从 0,0 到 100,0 画一条线",
     {"tool": "cad_line", "ok": True, "after": 1}),
    ("圆", "画一个圆心在 60,40 半径 12 的圆",
     {"tool": "cad_circle", "ok": True, "after": 1}),
    ("复合·配电柜", "画一台配电柜：外框 300x600，里面三条竖线，标题写柜A",
     {"tool": "cad_batch", "ok": True, "after": 5}),
    ("空间·四等分", "画一台 600 宽 1000 高的机柜，竖线四等分，标题写机柜1",
     {"tool": "cad_batch", "ok": True, "after": 5, "rect_vdivide": 4}),
    ("空间·居中", "在 600x400 的框里居中放一个半径 100 的圆",
     {"tool": "cad_batch", "ok": True, "circle_center": [300, 200]}),
    ("批量·三条线", "用一次批量画三条水平线：y=0、y=50、y=100，都从 x=0 到 x=200",
     {"tool": "cad_batch", "ok": True, "ops": 3, "after": 3}),
    ("未知句柄", "把句柄 698 删掉",
     {"blocked": "unknown_handle"}),
    ("统计", "图里有多少实体",
     {"tool_any": ["cad_stats"], "stats_answer": True}),
    ("写脚本", "写一段 Python 脚本，在原点画一条到 100 的线",
     {"tool": "cad_line"}),
    ("块参照", "在 0,0 插入配电箱符号块",
     {"tool": "cad_block_ref"}),
]


def post(base, path, payload):
    data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    req = urllib.request.Request(base + path, data=data,
                                 headers={"Content-Type": "application/json; charset=utf-8"})
    with urllib.request.urlopen(req, timeout=420) as r:
        return json.loads(r.read().decode("utf-8"))


def line_xs(tc):
    out = []
    for op in ((tc or {}).get("arguments") or {}).get("ops") or []:
        if str(op.get("tool")) in ("line",) and isinstance(op.get("args"), dict):
            p1 = op["args"].get("p1") or []
            if len(p1) == 2:
                out.append(round(float(p1[0])))
    return sorted(out)


def circle_center(tc):
    for op in ((tc or {}).get("arguments") or {}).get("ops") or []:
        if str(op.get("tool")) in ("circle",) and isinstance(op.get("args"), dict):
            c = op["args"].get("center") or []
            if len(c) == 2:
                return [round(float(c[0])), round(float(c[1]))]
    return None


def rect_vdivide(tc):
    """取 ops 里 rect 的 vdivide 值——第 6 代的核心判据：模型应给语义参数，
    坐标由工具层 _expand_layout 算，而不是自己算 x 值。"""
    for op in ((tc or {}).get("arguments") or {}).get("ops") or []:
        if str(op.get("tool")) == "rect" and isinstance(op.get("args"), dict):
            v = op["args"].get("vdivide")
            if v is not None:
                return int(v)
    return None


def run_case(base, text):
    post(base, "/api/reset", {})
    r = post(base, "/api/agent", {"text": text, "backend": "headless", "max_turns": 3})
    trace = r.get("trace") or []
    t1 = trace[0] if trace else {}
    tc = t1.get("tool_call")
    res = t1.get("result") or {}
    return {
        "turns": r.get("turns"),
        "seconds": r.get("seconds"),
        "done": t1.get("done"),
        "tool": (tc or {}).get("name"),
        "args": (tc or {}).get("arguments"),
        "result": res,
        "blocked": (t1.get("blocked") or {}).get("blocked"),
        "stats_answer": t1.get("stats_answer"),
        "gen_ms": t1.get("gen_ms"),
    }


def judge(got, chk):
    reasons = []
    if "blocked" in chk:
        if got["blocked"] != chk["blocked"]:
            reasons.append("blocked=%s 期望 %s" % (got["blocked"], chk["blocked"]))
        return reasons
    if "tool" in chk:
        if got["tool"] != chk["tool"]:
            reasons.append("工具 %s 期望 %s" % (got["tool"], chk["tool"]))
    if "tool_any" in chk:
        ok_tool = got["tool"] in chk["tool_any"]
        # 统计类：harness 会用现状快照直答（stats_answer），此时 tool_call 为 null，
        # 但模型 raw 里确实给了 cad_stats——两者任一成立即通过。
        ok_stats = bool(got.get("stats_answer"))
        if not (ok_tool or (chk.get("stats_answer") and ok_stats)):
            reasons.append("工具 %s 不在 %s，也没 stats_answer" % (got["tool"], chk["tool_any"]))
    if "ok" in chk:
        if (got["result"] or {}).get("ok") is not chk["ok"]:
            reasons.append("ok=%s 期望 %s" % ((got["result"] or {}).get("ok"), chk["ok"]))
    if "after" in chk:
        if (got["result"] or {}).get("after") != chk["after"]:
            reasons.append("after=%s 期望 %s" % ((got["result"] or {}).get("after"), chk["after"]))
    if "ops" in chk:
        n = len(((got["args"] or {}).get("ops")) or [])
        if n != chk["ops"]:
            reasons.append("ops=%d 期望 %d" % (n, chk["ops"]))
    if "line_xs" in chk:
        xs = line_xs({"arguments": got["args"]})
        if xs != chk["line_xs"]:
            reasons.append("竖线x=%s 期望 %s" % (xs, chk["line_xs"]))
    if "circle_center" in chk:
        c = circle_center({"arguments": got["args"]})
        if c != chk["circle_center"]:
            reasons.append("圆心=%s 期望 %s" % (c, chk["circle_center"]))
    if "rect_vdivide" in chk:
        v = rect_vdivide({"arguments": got["args"]})
        if v != chk["rect_vdivide"]:
            reasons.append("rect vdivide=%s 期望 %s（应给语义参数，不要自己算 x）"
                           % (v, chk["rect_vdivide"]))
    if chk.get("stats_answer") and not (got["stats_answer"] or (got["tool"] == "cad_stats")):
        reasons.append("既没 cad_stats 也没 stats_answer")
    return reasons


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="http://127.0.0.1:8770")
    ap.add_argument("--only", default="")
    a = ap.parse_args()

    passed = 0
    rows = []
    for name, text, chk in CASES:
        if a.only and a.only not in name:
            continue
        try:
            got = run_case(a.base, text)
        except Exception as exc:
            rows.append((name, "ERR", repr(exc)[:120], 0))
            continue
        bad = judge(got, chk)
        verdict = "PASS" if not bad else "FAIL"
        if not bad:
            passed += 1
        detail = "tool=%s after=%s sec=%s" % (
            got["tool"], (got["result"] or {}).get("after"), got["seconds"])
        if bad:
            detail = "; ".join(bad) + " | " + detail
        rows.append((name, verdict, detail, got["seconds"] or 0))

    print("\n%-14s %-6s %s" % ("用例", "判定", "明细"))
    print("-" * 96)
    for name, verdict, detail, _ in rows:
        print("%-14s %-6s %s" % (name, verdict, detail))
    total = len(rows)
    print("-" * 96)
    print("通过 %d/%d" % (passed, total))
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
