# -*- coding: utf-8 -*-
"""gen_batch.py —— 按批增量生成训练数据并即时质检（给定时任务用）。

复用 gen_dataset_100k.py 的生成池，换随机种子产出新样本，做四道质检后
追加到 sft_stream.jsonl；重复样本丢弃，质检不过的丢弃并计数。

用法：python gen_batch.py [--rows 2000] [--out sft_stream.jsonl]
"""
import argparse
import hashlib
import json
import os
import random
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import gen_dataset_100k as G          # 复用生成池
import verify_dataset as V            # 复用质检口径（工具集 / api_index）

MAXCHARS = 700


def _sig(sample):
    msgs = sample.get("messages") or []
    parts = []
    for m in msgs:
        parts.append((m.get("role") or "") + ":" + (m.get("content") or ""))
        for tc in (m.get("tool_calls") or []):
            fn = tc.get("function") or {}
            parts.append("call:" + (fn.get("name") or "") + ":" + (fn.get("arguments") or ""))
    return hashlib.sha1("\x00".join(parts).encode("utf-8")).hexdigest()


def load_seen_keys(paths):
    keys = set()
    total = 0
    for p in paths:
        if not os.path.exists(p):
            continue
        with open(p, encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                total += 1
                try:
                    keys.add(_sig(json.loads(line)))
                except Exception:
                    pass
    return keys, total


def check(sample, api_pairs):
    """返回 None 表示合格，否则返回失败原因。"""
    msgs = sample.get("messages") or []
    if len(msgs) != 3 or [m.get("role") for m in msgs] != ["system", "user", "assistant"]:
        return "messages 结构不对"
    asst = msgs[2]
    text_parts = [m.get("content") or "" for m in msgs]
    if asst.get("tool_calls"):
        if len(asst["tool_calls"]) != 1:
            return "tool_calls 数量异常"
        fn = asst["tool_calls"][0].get("function") or {}
        name = fn.get("name")
        if name not in V.REAL_TOOLS:
            return "工具名不存在:%s" % name
        try:
            args = json.loads(fn.get("arguments") or "{}")
            if not isinstance(args, dict):
                return "arguments 不是对象"
        except Exception:
            return "arguments 非法 JSON"
        text_parts.append(fn.get("arguments") or "")
    total_chars = sum(len(t) for t in text_parts)
    if total_chars > MAXCHARS:
        return "超长 %d 字符" % total_chars
    if (sample.get("meta") or {}).get("kind") == "api":
        body = asst.get("content") or ""
        for cls, meth in re.findall(r"#?\s*(\w+)\.(\w+)", body):
            if cls.startswith(("Ge", "Db", "P")) and any(p[0] == cls for p in api_pairs) \
                    and (cls, meth) not in api_pairs:
                return "API 不存在:%s.%s" % (cls, meth)
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rows", type=int, default=2000)
    ap.add_argument("--out", default=os.path.join(HERE, "sft_stream.jsonl"))
    a = ap.parse_args()

    t0 = time.time()
    seed = int.from_bytes(os.urandom(4), "big")
    G.R = random.Random(seed)
    G.VISIBLE.clear()
    G._seen.clear()

    n = a.rows

    def wave(count):
        """生成一批候选；G._seen 跨批保留，避免同一进程内自己重复。"""
        G.VISIBLE.clear()
        G.gen_tools(int(count * 0.45))
        G.gen_api(int(count * 0.30))
        G.gen_entity(int(count * 0.15))
        G.gen_geo(int(count * 0.08))
        G.gen_correct(max(1, count - len(G.VISIBLE)))
        if len(G.VISIBLE) < count:
            G.gen_geo(count - len(G.VISIBLE) + 50)
        return list(G.VISIBLE)

    base = os.path.join(HERE, "sft_100k.jsonl")
    seen, base_total = load_seen_keys([base, a.out])
    api_pairs = V.real_api_set()

    kept, dup, bad = [], 0, []
    cnt = {}
    quota = {"tool": int(n * 0.45), "api": int(n * 0.30), "entity": int(n * 0.15),
             "geo": int(n * 0.08)}
    quota["correct"] = max(1, n - sum(quota.values()))

    def harvest(samples, enforce):
        nonlocal dup
        added = 0
        for s in samples:
            if len(kept) >= n:
                break
            kind = (s.get("meta") or {}).get("kind")
            if enforce and cnt.get(kind, 0) >= quota.get(kind, 0):
                continue
            k = _sig(s)
            if k in seen:
                dup += 1
                continue
            err = check(s, api_pairs)
            if err:
                bad.append(err)
                continue
            seen.add(k)
            kept.append(s)
            cnt[kind] = cnt.get(kind, 0) + 1
            added += 1
        return added

    rounds = 0
    # 第 1 阶段按配比收，第 2 阶段缺口才允许某一类溢出
    while len(kept) < n and rounds < 12:
        rounds += 1
        harvest(wave(max((n - len(kept)) * 2, 400)), enforce=(rounds <= 6))

    os.makedirs(os.path.dirname(a.out) or HERE, exist_ok=True)
    with open(a.out, "a", encoding="utf-8") as f:
        for s in kept:
            f.write(json.dumps(s, ensure_ascii=False) + "\n")

    kinds = {}
    for s in kept:
        kk = (s.get("meta") or {}).get("kind")
        kinds[kk] = kinds.get(kk, 0) + 1
    print("[gen] ts=%s seed=%d 合格=%d/%d 重复=%d 质检失败=%d 批数=%d 池总=%d 库总=%d %.1fs %s"
          % (time.strftime("%Y-%m-%d %H:%M:%S"), seed, len(kept), n, dup, len(bad), rounds,
             base_total, len(seen), time.time() - t0,
             json.dumps(kinds, ensure_ascii=False)), flush=True)
    if bad:
        uniq = {}
        for e in bad:
            uniq[e] = uniq.get(e, 0) + 1
        print("[gen] 失败原因 " + json.dumps(uniq, ensure_ascii=False), flush=True)
    return 0 if kept else 1


if __name__ == "__main__":
    raise SystemExit(main())
