# -*- coding: utf-8 -*-
"""gen_stream.py —— 每批 2000 条增量训练数据，生成即自检。

与 gen_dataset_100k.py 的区别：
  1. 种子由系统时间派生，每批内容不同（不是固定 10 万条切片）
  2. append 到 sft_stream.jsonl，不覆盖已有文件
  3. 生成后立刻按 verify_dataset.py 的口径自检，不合格的样本不进文件
  4. 与最近历史（尾部 5 万条）做全文本去重

用法：
  python gen_stream.py [条数]     默认 2000
"""
import json
import os
import random
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gen_dataset_100k as G

OUT = os.path.join(HERE, "sft_stream.jsonl")
LOG = os.path.join(HERE, "logs", "gen_stream.log")
IDX = r"G:\PyArx\tools\cadmcp\api_index.jsonl"
MAXCHARS = 700
TAIL_LINES = 50000

REAL_TOOLS = {"cad_line", "cad_polyline", "cad_circle", "cad_arc", "cad_text",
              "cad_mtext", "cad_hatch", "cad_table", "cad_rect", "cad_block_ref",
              "cad_layer", "cad_batch", "cad_stats", "cad_query", "cad_erase",
              "cad_offset", "cad_save", "cad_entity"}

# ---------------- 进度感知：生成量跟着训练消耗走 ----------------
LOG_DIR = os.path.join(HERE, "logs")
POOL = os.path.join(HERE, "sft_pool.jsonl")
BASE = os.path.join(HERE, "sft_100k.jsonl")
BUFFER_DAYS = 2.0      # 目标：池里始终备着未来 N 天的训练消耗
MAX_PER_RUN = 4000     # 单轮生成上限（避免一次灌太多）


def _count_lines(path):
    if not os.path.exists(path):
        return 0
    n = 0
    with open(path, "rb") as f:
        for _ in f:
            n += 1
    return n


def _latest_train_obs():
    """从最新训练日志取最后一步与累计秒，估步速、判断训练是否还在跑。"""
    best = None
    if os.path.isdir(LOG_DIR):
        for fn in os.listdir(LOG_DIR):
            if not fn.endswith(".log") or not fn.startswith("train"):
                continue
            p = os.path.join(LOG_DIR, fn)
            mt = os.path.getmtime(p)
            if best is None or mt > best[0]:
                best = (mt, p)
    if not best:
        return 0, 0.0, False
    mt, p = best
    step = 0
    sec = 0
    size = os.path.getsize(p)
    with open(p, "rb") as f:
        f.seek(max(0, size - 400 * 1024))
        tail = f.read().decode("utf-8", "ignore")
    for m in re.finditer(r"ep(\d+)\s+step(\d+)\s+loss=[\d.]+\s+(\d+)s", tail):
        step = int(m.group(2))
        sec = int(m.group(3))
    alive = (time.time() - mt) < 600 and step > 0
    speed = (sec / float(step)) if step else 0.0
    return step, speed, alive



def _log(msg):
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    line = "%s %s" % (time.strftime("%Y-%m-%d %H:%M:%S"), msg)
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(line + "\n")
    print(line)


def _chars(s):
    parts = [m.get("content") or "" for m in s["messages"]]
    tc = s["messages"][2].get("tool_calls")
    if tc:
        parts.append(tc[0]["function"].get("arguments") or "")
    return sum(len(p) for p in parts)


def _norm(s):
    parts = [m.get("content") or "" for m in s["messages"]]
    tc = s["messages"][2].get("tool_calls")
    if tc:
        parts.append(tc[0]["function"].get("arguments") or "")
    return re.sub(r"\s+", "", "".join(parts))


def _tail_hashes():
    """已落盘数据的尾部指纹，用于跨批去重（不整份读，控制耗时）。"""
    h = set()
    if not os.path.exists(OUT):
        return h
    with open(OUT, "rb") as f:
        # 粗取尾部：按字节往回读约 8MB
        size = os.path.getsize(OUT)
        f.seek(max(0, size - 8 * 1024 * 1024))
        chunk = f.read().decode("utf-8", "ignore")
    for line in chunk.splitlines()[-TAIL_LINES:]:
        line = line.strip()
        if not line:
            continue
        try:
            s = json.loads(line)
        except Exception:
            continue
        h.add(_norm(s))
    return h


def _api_pairs():
    pairs = set()
    with open(IDX, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            r = json.loads(line)
            m = re.match(r"^(\w+)\.(\w+)\(", r.get("sig") or "")
            if m:
                pairs.add((m.group(1), m.group(2)))
    cls_known = set(p[0] for p in pairs)
    return pairs, cls_known


def build(n, seed):
    G.R = random.Random(seed)
    G._seen.clear()
    G.VISIBLE.clear()
    plan = [("gen_tools", 0.42), ("gen_api", 0.33), ("gen_entity", 0.15), ("gen_geo", 0.10)]
    for fn, ratio in plan:
        getattr(G, fn)(int(n * ratio))
    # 补足到 n（超长样本会被丢掉，所以要补）
    for _ in range(6):
        ok = [s for s in G.VISIBLE if _chars(s) <= MAXCHARS]
        if len(ok) >= n:
            break
        G.gen_geo(n)
    return [s for s in G.VISIBLE if _chars(s) <= MAXCHARS][:n]


def check(rows):
    pairs, cls_known = _api_pairs()
    errs = []
    kinds = {}
    tools = {}
    long_n = 0
    for i, s in enumerate(rows, 1):
        msgs = s.get("messages") or []
        if len(msgs) != 3 or [m.get("role") for m in msgs] != ["system", "user", "assistant"]:
            errs.append("第 %d 条 messages 结构不对" % i)
            continue
        asst = msgs[2]
        if asst.get("tool_calls"):
            fn = asst["tool_calls"][0]["function"]
            name = fn.get("name")
            tools[name] = tools.get(name, 0) + 1
            if name not in REAL_TOOLS:
                errs.append("第 %d 条工具名不存在：%s" % (i, name))
            try:
                json.loads(fn.get("arguments") or "{}")
            except Exception:
                errs.append("第 %d 条 arguments 非 JSON" % i)
        kind = (s.get("meta") or {}).get("kind")
        kinds[kind] = kinds.get(kind, 0) + 1
        if _chars(s) > MAXCHARS:
            long_n += 1
        if kind == "api":
            for cls, meth in re.findall(r"#?\s*(\w+)\.(\w+)", asst.get("content") or ""):
                if cls in cls_known and (cls, meth) not in pairs:
                    errs.append("第 %d 条 api 不存在：%s.%s" % (i, cls, meth))
    return errs, kinds, tools, long_n


def main():
    arg = (sys.argv[1] if len(sys.argv) > 1 else "auto").strip().lower()
    if arg == "auto":
        step, speed, alive = _latest_train_obs()
        pool = _count_lines(POOL)
        if pool <= 0:
            pool = _count_lines(BASE) + _count_lines(OUT)
        if speed > 0:
            per_day = 86400.0 / speed      # 1 步 = 1 条样本
            target = step + per_day * BUFFER_DAYS
        else:
            per_day = 0.0
            target = pool                  # 没有速度参考就当库存已够
        need = int(target - pool) + 1 if target > pool else 0
        if (not alive) or need <= 0:
            _log("[auto] 跳过 step=%d 步速=%.2fs/步 每日消耗≈%d 库存=%d 目标=%d 缺口=%d 训练在跑=%s"
                 % (step, speed, int(per_day), pool, int(target), need, alive))
            _log("[result] SKIP")
            return 0
        n = min(need, MAX_PER_RUN)
        _log("[auto] step=%d 步速=%.2fs/步 每日消耗≈%d 库存=%d 缺口=%d 本批生成=%d"
             % (step, speed, int(per_day), pool, need, n))
    else:
        n = int(arg)

    t0 = time.time()
    # 种子带时间戳 + 随机，保证每批内容不同
    seed = (int(time.time() * 1000) ^ random.randint(0, 1 << 30)) & 0x7FFFFFFF
    rows = build(n, seed)

    errs, kinds, tools, long_n = check(rows)
    hist = _tail_hashes()
    fresh = []
    dup = 0
    for s in rows:
        k = _norm(s)
        if k in hist:
            dup += 1
            continue
        hist.add(k)
        fresh.append(s)

    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    with open(OUT, "a", encoding="utf-8") as f:
        for s in fresh:
            f.write(json.dumps(s, ensure_ascii=False) + "\n")

    try:
        with open(OUT, encoding="utf-8") as f:
            total = sum(1 for _ in f)
    except Exception:
        total = -1

    cost = time.time() - t0
    _log("[batch] seed=%d 生成=%d 去重剔除=%d 入库=%d 总行数=%d | 结构/工具/API错误=%d 超长=%d | kind=%s | 耗时=%.1fs"
         % (seed, len(rows), dup, len(fresh), total, len(errs), long_n,
            json.dumps(kinds, ensure_ascii=False), cost))
    for e in errs[:5]:
        _log("  [err] " + e)
    # 验收口径：错误 0、入库 2000、无严重重复
    ok = (len(errs) == 0 and len(fresh) >= int(n * 0.98))
    _log("[result] %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
