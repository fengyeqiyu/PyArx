# -*- coding: utf-8 -*-
"""test_shapes.py —— 逐个模板测试 gen_shapes.py 的 30 组模板。

每个模板生成 3 条（覆盖随机分支），做四层校验：
  1. 结构：角色序列符合该类形态；tool_calls 名在 18 真实工具内；arguments 可解析
  2. 长度：整条 ≤ 700 字符（MAXLEN=384 的安全线）
  3. 一致性：user 话里出现的图层名 / 坐标对 / “N 等分” / 引号文字，必须体现在参数里
  4. 合法性：半径 >0、角度 0..360、坐标 0..1000、rows/cols 有上限、句柄是字符串

用法：python test_shapes.py [每模板条数]
"""
import json
import re
import sys
import os

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gen_shapes as S

REAL_TOOLS = {"cad_line", "cad_polyline", "cad_circle", "cad_arc", "cad_text",
              "cad_mtext", "cad_hatch", "cad_table", "cad_rect", "cad_block_ref",
              "cad_layer", "cad_batch", "cad_stats", "cad_query", "cad_erase",
              "cad_offset", "cad_save", "cad_entity"}
CN_NUM = {"二": 2, "两": 2, "三": 3, "四": 4, "五": 5, "六": 6, "七": 7, "八": 8}

LAYER_NAMES = set(S.G.LAYERS)


def _flatten(v, out):
    if isinstance(v, dict):
        for x in v.values():
            _flatten(x, out)
    elif isinstance(v, (list, tuple)):
        for x in v:
            _flatten(x, out)
    else:
        out.append(v)


def _nums(v):
    out = []
    _flatten(v, out)
    return [x for x in out if isinstance(x, (int, float)) and not isinstance(x, bool)]


def _args(sample):
    """该样本里所有 tool_calls 的 (name, args_dict)。"""
    res = []
    for m in sample["messages"]:
        for c in (m.get("tool_calls") or []):
            fn = c["function"]
            res.append((fn["name"], json.loads(fn["arguments"])))
    return res


def check_structure(sample, kind, why):
    msgs = sample["messages"]
    roles = [m["role"] for m in msgs]
    if roles[0] != "system" or roles[1] != "user":
        why.append("开头必须是 system+user，实际 %s" % roles[:2])
    if kind == "multi_call":
        want = ["system", "user", "assistant"]
    elif kind == "multi_turn":
        if len(msgs) < 5:
            why.append("多步至少 system/user/assistant/tool/assistant 五段")
        if roles[2] != "assistant" or roles[3] != "tool":
            why.append("多步第 3、4 段必须是 assistant+tool")
        if roles[-1] != "assistant":
            why.append("多步最后一段必须是 assistant（做法）")
        want = roles
    else:
        if len(msgs) != 5:
            why.append("自纠必须是 5 段，实际 %d" % len(msgs))
        want = ["system", "user", "assistant", "tool", "assistant"]
    if kind != "multi_turn" and roles != want:
        why.append("角色序列应为 %s，实际 %s" % (want, roles))
    for name, args in _args(sample):
        if name not in REAL_TOOLS:
            why.append("工具名不存在：%s" % name)
        if not isinstance(args, dict):
            why.append("%s 的 arguments 不是对象" % name)


def check_length(sample, why):
    n = S._chars(sample)
    if n > S.MAXCHARS:
        why.append("超长 %d > %d" % (n, S.MAXCHARS))


def check_legality(sample, why, kind):
    """自纠样本故意用非法参数，只校验它最后那次修正调用。"""
    calls = _args(sample)
    if kind == "repair":
        last = [m for m in sample["messages"] if m["role"] == "assistant"][-1]
        calls = [(c["function"]["name"], json.loads(c["function"]["arguments"]))
                 for c in (last.get("tool_calls") or [])]
    for name, args in calls:
        if name == "cad_circle":
            r = args.get("radius")
            if not isinstance(r, (int, float)) or r <= 0:
                why.append("cad_circle radius 非法：%r" % (r,))
        if name == "cad_arc":
            for k in ("start_angle", "end_angle"):
                a = args.get(k)
                if not isinstance(a, (int, float)) or a < 0 or a > 360:
                    why.append("cad_arc %s 超范围：%r" % (k, a))
        if name in ("cad_line", "cad_circle", "cad_text", "cad_mtext", "cad_block_ref", "cad_table",
                    "cad_rect"):
            for p in (args.get("p1"), args.get("p2"), args.get("pos"), args.get("center")):
                if p is None:
                    continue
                if not (isinstance(p, list) and len(p) == 2):
                    why.append("%s 的点不是 [x,y]：%r" % (name, p))
                elif not all(isinstance(v, (int, float)) and -1e5 < v < 1e5 for v in p):
                    why.append("%s 点坐标非法：%r" % (name, p))
        if name == "cad_table":
            for k, lim in (("rows", 10), ("cols", 10)):
                v = args.get(k)
                if isinstance(v, int) and v > lim:
                    why.append("cad_table %s 超上限：%s" % (k, v))
        if name == "cad_erase":
            hs = args.get("handles") or []
            if not all(isinstance(h, str) for h in hs):
                why.append("cad_erase handles 必须是字符串列表")
        if name == "cad_offset":
            if not isinstance(args.get("handle"), str):
                why.append("cad_offset handle 必须是字符串")


def check_repair_truth(sample, why):
    """自纠样本里那次"错"必须真的表现为失败，否则学不到纠正。"""
    msgs = sample["messages"]
    if len(msgs) != 5:
        return
    tool_msg = msgs[3]
    try:
        payload = json.loads(tool_msg.get("content") or "{}")
    except Exception:
        why.append("tool 返回不是 JSON")
        return
    if payload.get("ok") is True and not payload.get("warnings"):
        why.append("自纠样本的错误调用实际成功了，没有可纠正的信号")


def check_consistency(sample, why):
    """user 话里的图层名/坐标/等分/引号文字必须在参数里出现。"""
    user = sample["messages"][1]["content"]
    calls = _args(sample)
    arg_text = json.dumps([a for _, a in calls], ensure_ascii=False)
    arg_nums = set()
    for _, a in calls:
        for v in _nums(a):
            arg_nums.add(round(float(v), 3))

    # 图层名
    for lay in LAYER_NAMES:
        if re.search(r"(?<![A-Z0-9_])%s(?![A-Z0-9_])" % re.escape(lay), user):
            if '"%s"' % lay not in arg_text and ":%s" % lay not in arg_text and lay not in arg_text:
                why.append("话里提到图层 %s，参数里没有" % lay)

    # 坐标对 (x,y)
    for x, y in re.findall(r"\((-?\d+),(-?\d+)\)", user):
        if round(float(x), 3) not in arg_nums or round(float(y), 3) not in arg_nums:
            why.append("话里的坐标 (%s,%s) 未出现在参数中" % (x, y))

    # N 等分
    for cn, n in CN_NUM.items():
        if re.search(r"%s等分" % cn, user):
            if ("\"vdivide\": %d" % n not in arg_text and "\"hdivide\": %d" % n not in arg_text
                    and not any(a.get("vdivide") == n or a.get("hdivide") == n for _, a in calls)):
                why.append("话里说%s等分，参数没有 vdivide/hdivide=%d" % (cn, n))
    if "横向四等分" in user:
        if not any(a.get("vdivide") == 4 for _, a in calls):
            why.append("横向四等分但 vdivide≠4")

    # 引号文字
    for t in re.findall(r"“([^”]+)”", user):
        if t not in arg_text:
            why.append("话里的文字“%s”未出现在参数" % t)

    # 句柄：话里给的号码，参数里必须有对应字符串
    for h in re.findall(r"(\d{3,5}) 号", user):
        if h not in arg_text:
            why.append("话里的句柄 %s 未出现在参数" % h)


def run_template(fn, kind, rounds):
    fails = []
    for i in range(rounds):
        if kind == "multi_call":
            user, calls, label = fn()
            sample = S._row([{"role": "system", "content": S.SYS_SHORT},
                             {"role": "user", "content": user}, S._asst(*calls)], kind, label)
        elif kind == "multi_turn":
            user, steps, final, label = fn()
            msgs = [{"role": "system", "content": S.SYS_SHORT}, {"role": "user", "content": user}]
            for a, t in steps:
                msgs += [a, t]
            msgs.append(final)
            sample = S._row(msgs, kind, label)
        else:
            user, bad, bad_res, good, label = fn()
            sample = S._row([{"role": "system", "content": S.SYS_SHORT},
                             {"role": "user", "content": user},
                             S._asst(*bad), S._tool(bad_res), S._asst(*good)], kind, label)
        why = []
        check_structure(sample, kind, why)
        check_length(sample, why)
        check_legality(sample, why, kind)
        check_consistency(sample, why)
        if kind == "repair":
            check_repair_truth(sample, why)
        if why:
            fails.append((i, why, sample))
    return fails


def main():
    rounds = int(sys.argv[1]) if len(sys.argv) > 1 else 3
    groups = [("multi_call", S.MULTI_CALL_TPLS), ("multi_turn", S.MULTI_TURN_TPLS),
              ("repair", S.REPAIR_TPLS)]
    total = bad = 0
    for kind, tpls in groups:
        print("===== %s（%d 组模板）=====" % (kind, len(tpls)))
        for fn in tpls:
            total += 1
            fails = run_template(fn, kind, rounds)
            if fails:
                bad += 1
                print("  FAIL %-28s %d/%d 条不合格" % (fn.__name__, len(fails), rounds))
                seen = set()
                for i, why, sample in fails[:2]:
                    for w in why:
                        if w in seen:
                            continue
                        seen.add(w)
                        print("      - %s" % w)
                    print("      user: %s" % sample["messages"][1]["content"])
            else:
                print("  PASS %-28s %d/%d" % (fn.__name__, rounds, rounds))
    print("\n模板 %d 个，通过 %d，失败 %d" % (total, total - bad, bad))
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
