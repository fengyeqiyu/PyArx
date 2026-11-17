# -*- coding: utf-8 -*-
"""merge_stream.py —— 把增量池并入训练池，训练启动前跑一次。

输入：
  sft_100k.jsonl    固定 10 万条基础集（原样保留，绝不改）
  sft_stream.jsonl  每 5 分钟 +2000 条的增量集
输出：
  sft_pool.jsonl    两份合并去重后的训练用文件（训练脚本指向这里）

去重口径与 gen_stream.py 一致：system+user+content+arguments 去掉空白后的全文。
流式读写，内存只占一份指纹集合。
"""
import json
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.join(HERE, "sft_100k.jsonl")
STREAM = os.path.join(HERE, "sft_stream.jsonl")
SHAPES = os.path.join(HERE, "sft_shapes.jsonl")
POOL = os.path.join(HERE, "sft_pool.jsonl")
LOG = os.path.join(HERE, "logs", "merge_stream.log")


def _norm(s):
    parts = [m.get("content") or "" for m in s["messages"]]
    tc = s["messages"][2].get("tool_calls")
    if tc:
        parts.append(tc[0]["function"].get("arguments") or "")
    return re.sub(r"\s+", "", "".join(parts))


def _log(msg):
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    line = "%s %s" % (time.strftime("%Y-%m-%d %H:%M:%S"), msg)
    with open(LOG, "a", encoding="utf-8") as f:
        f.write(line + "\n")
    print(line)


def main():
    t0 = time.time()
    seen = set()
    n_src = 0
    n_dup = 0
    n_out = 0
    bad = 0
    kinds = {}
    tmp = POOL + ".tmp"
    with open(tmp, "w", encoding="utf-8") as fout:
        for path in (BASE, STREAM, SHAPES):
            if not os.path.exists(path):
                continue
            with open(path, encoding="utf-8") as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue
                    n_src += 1
                    try:
                        s = json.loads(line)
                    except Exception:
                        bad += 1
                        continue
                    k = _norm(s)
                    if k in seen:
                        n_dup += 1
                        continue
                    seen.add(k)
                    fout.write(json.dumps(s, ensure_ascii=False) + "\n")
                    n_out += 1
                    kd = (s.get("meta") or {}).get("kind")
                    kinds[kd] = kinds.get(kd, 0) + 1
    # 原子替换，避免训练读到写了一半的文件
    if os.path.exists(POOL):
        os.remove(POOL)
    os.rename(tmp, POOL)
    size_mb = os.path.getsize(POOL) / 1048576.0
    _log("[merge] 基础集+增量集=%d 去重剔除=%d 坏行=%d -> %d 条 (%.1f MB) kind=%s 耗时=%.1fs"
         % (n_src, n_dup, bad, n_out, size_mb, json.dumps(kinds, ensure_ascii=False),
            time.time() - t0))
    if bad:
        _log("[warn] %d 行解析失败，已跳过" % bad)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
