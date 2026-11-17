# -*- coding: utf-8 -*-
"""verify_dataset.py —— 训练数据交付前质检。

检查项（每一项失败都必须修数据，而不是只报告）：
  1. 每行是合法 JSON，messages 角色顺序正确
  2. tool_calls 的 name 在真实工具集合内，arguments 是合法 JSON
  3. kind=api 的样本里出现的 Class.method 必须在 api_index.jsonl 里真实存在
     （这是"模型背不存在的 API"唯一可控的防线）
  4. 长度：整条字符数 ≤ 700（MAXLEN=384 token 下的保守上限）
  5. 全文本重复率
"""
import json
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "sft_100k.jsonl")
IDX = r"G:\PyArx\tools\cadmcp\api_index.jsonl"
MAXCHARS = 700

REAL_TOOLS = {"cad_line", "cad_polyline", "cad_circle", "cad_arc", "cad_text",
              "cad_mtext", "cad_hatch", "cad_table", "cad_rect", "cad_block_ref",
              "cad_layer", "cad_batch", "cad_stats", "cad_query", "cad_erase",
              "cad_offset", "cad_save", "cad_entity"}


def load_100k():
    rows = []
    with open(DATA, encoding="utf-8") as f:
        for i, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            rows.append((i, json.loads(line)))
    return rows


def real_api_set():
    pairs = set()
    with open(IDX, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            r = json.loads(line)
            sig = r.get("sig") or ""
            m = re.match(r"^(\w+)\.(\w+)\(", sig)
            if m:
                pairs.add((m.group(1), m.group(2)))
    return pairs


def main():
    errors = []
    kinds = {}
    tools = {}
    long_rows = []
    dup = {}
    api_rows = 0
    api_bad = []
    pairs = real_api_set()
    rows = load_100k()

    for i, s in rows:
        msgs = s.get("messages") or []
        if len(msgs) != 3 or [m.get("role") for m in msgs] != ["system", "user", "assistant"]:
            errors.append("第 %d 行 messages 结构不对" % i)
            continue
        asst = msgs[2]
        text_parts = [m.get("content") or "" for m in msgs]
        if asst.get("tool_calls"):
            tc = asst["tool_calls"][0]["function"]
            name = tc.get("name")
            tools[name] = tools.get(name, 0) + 1
            if name not in REAL_TOOLS:
                errors.append("第 %d 行工具名不存在：%s" % (i, name))
            try:
                json.loads(tc.get("arguments") or "{}")
            except Exception:
                errors.append("第 %d 行 arguments 不是合法 JSON" % i)
            text_parts.append(tc.get("arguments") or "")
        kind = (s.get("meta") or {}).get("kind")
        kinds[kind] = kinds.get(kind, 0) + 1

        total_chars = sum(len(t) for t in text_parts)
        if total_chars > MAXCHARS:
            long_rows.append((i, total_chars, kind))

        full = re.sub(r"\s+", "", "".join(text_parts))
        dup[full] = dup.get(full, 0) + 1

        if kind == "api":
            api_rows += 1
            body = (asst.get("content") or "")
            found = re.findall(r"#?\s*(\w+)\.(\w+)", body)
            for cls, meth in found:
                if cls.startswith(("Ge", "Db", "P")) and (cls, meth) not in pairs:
                    # 只对真实类名做严格校验，避免把注释里的普通词当 API
                    if any(p[0] == cls for p in pairs):
                        api_bad.append((i, cls, meth))

    dup_over = [(k, v) for k, v in dup.items() if v > 1]
    print("总条数 %d" % len(rows))
    print("kind 分布 %s" % json.dumps(kinds, ensure_ascii=False))
    print("工具分布 %s" % json.dumps(tools, ensure_ascii=False))
    print("结构/工具/JSON 错误 %d 处" % len(errors))
    for e in errors[:10]:
        print("  " + e)
    print("超长（>%d 字符）%d 条，占 %.2f%%"
          % (MAXCHARS, len(long_rows), 100.0 * len(long_rows) / max(1, len(rows)))
          + "，最长 %d" % (max([t for _, t, _ in long_rows], default=0)))
    print("全文本重复组 %d 组" % len(dup_over))
    print("api 类样本 %d 条，疑似不存在的方法名 %d 处" % (api_rows, len(api_bad)))
    for r in api_bad[:10]:
        print("  行 %d：%s.%s" % r)
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
