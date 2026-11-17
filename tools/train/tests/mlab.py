# -*- coding: utf-8 -*-
"""模块自测断言收集器（在 accoreconsole 里跑，import PyArxCore 之后使用）。
统一口径：一个模块 = 一条 matrix 记录，四类断言：
  1) 存在性与构造   check: hasattr / 最少参数构造 / type() 实际类名
  2) 属性往返       check: set 后 get 读回一致
  3) 入库闭环       check: 事务内 new+addEntity → handle != "0" → commit 后 count 增 → 再读回
  4) 失败面         probe: 错参数的真实行为（抛出/返回False/静默不改），不判对错只记现象

落盘结构（与 module_matrix.json 一致）：
    [{"layer":..,"module":..,"checks":[{"name","ok","detail"}],"summary":{"pass","fail"}}]
"""

import json
import os
import traceback

ROWS = {}


def _put(layer, module, name, ok, detail):
    ROWS.setdefault((layer, module), []).append(
        {"name": name, "ok": bool(ok), "detail": str(detail)[:400]}
    )


def check(layer, module, name, fn):
    """跑一条断言：正常返回 → pass（返回值写进 detail）；抛异常 → fail（异常类型+信息）。"""
    try:
        detail = fn()
        _put(layer, module, name, True, detail)
        return True
    except Exception as e:
        line = traceback.format_exc().strip().splitlines()[-1]
        _put(layer, module, name, False, "%s | %s" % (type(e).__name__, line[:300]))
        return False


def ctor(cls, *forms):
    """按顺序试构造形式，返回 (实例, 命中的形参个数表述)；全失败抛最后一个异常。

    forms 每项是 tuple(位置参数)；无参用 ()。用来同时完成"最少参数构造"和签名探测。
    """
    last = None
    for args in forms:
        try:
            return cls(*args), ("args=%d" % len(args))
        except Exception as e:
            last = e
    raise last if last is not None else RuntimeError("no form")


def probe(layer, module, name, fn, allow=("Exception", "False", "None")):
    """失败面：调用 fn，记录真实行为。

    allow 是"可接受的失败形态"前缀集合，默认 异常/False/None。
    返回 (kind, repr)；若返回了一个像成功的结果（不在 allow 里）则判 fail，
    说明该 API 对错参数静默接受——这类结论对训练最值钱。
    """
    try:
        v = fn()
        kind, val = "return", repr(v)[:200]
    except Exception as e:
        kind, val = "Exception", "%s: %s" % (type(e).__name__, str(e)[:200])
    ok = any(kind.startswith(a) or val.startswith(a) for a in allow)
    _put(layer, module, name + " [probe]", ok,
         "%s -> %s" % (kind, val))
    return kind, val


def ent_count():
    """当前空间实体数（用于入库闭环前后对比）。"""
    return len(list(__import__("PyArxCore").Db.curDb().objectIds("AcDbEntity")))


def dump(path):
    """写出 matrix 片段；若文件已存在则按 layer+module 合并覆盖同名模块。"""
    out = []
    if os.path.isfile(path):
        try:
            with open(path, encoding="utf-8") as f:
                out = json.load(f) or []
        except Exception:
            out = []
    index = {(r.get("layer"), r.get("module")): i for i, r in enumerate(out)}
    for (layer, module), checks in ROWS.items():
        rec = {
            "layer": layer,
            "module": module,
            "checks": checks,
            "summary": {
                "pass": sum(1 for c in checks if c["ok"]),
                "fail": sum(1 for c in checks if not c["ok"]),
            },
        }
        if (layer, module) in index:
            out[index[(layer, module)]] = rec
        else:
            out.append(rec)
    d = os.path.dirname(path)
    if d and not os.path.isdir(d):
        os.makedirs(d)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False, indent=1)
    return {"path": path, "modules": len(out), "rows_written": len(ROWS)}


def report():
    """不落盘，直接返回内存里的行（写进结果 JSON 便于 console 侧核对）。"""
    return [
        {"layer": l, "module": m,
         "summary": {"pass": sum(1 for c in cs if c["ok"]),
                     "fail": sum(1 for c in cs if not c["ok"])},
         "checks": cs}
        for (l, m), cs in ROWS.items()
    ]


def out_path(fallback):
    """结果 JSON 路径：优先用 run.py 传进来的 PYARX_OUT，否则用 fallback。"""
    return os.environ.get("PYARX_OUT") or fallback


def typeerror_hint(cls, *bad):
    """用一组错参数点出 pybind 的"支持的构造/调用形式"原文（TypeError 消息里就是签名真相）。

    返回截断后的签名文本；若没有抛错说明这些参数居然可用，返回 "no TypeError"。
    """
    try:
        cls(*bad)
        return "no TypeError（这些参数可用）"
    except TypeError as e:
        return " ".join(str(e).split())[:600]
    except Exception as e:
        return "%s: %s" % (type(e).__name__, str(e)[:300])


def method_typeerror(obj, meth, *bad):
    """同上，针对某个方法（obj 需已构造成功）。"""
    fn = getattr(obj, meth, None)
    if fn is None:
        return "%s 不存在" % meth
    try:
        fn(*bad)
        return "no TypeError（这些参数可用）"
    except TypeError as e:
        return " ".join(str(e).split())[:600]
    except Exception as e:
        return "%s: %s" % (type(e).__name__, str(e)[:300])

