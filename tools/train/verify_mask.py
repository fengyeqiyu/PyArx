# -*- coding: utf-8 -*-
"""验证多轮样本的 loss 覆盖面：每个 assistant 段的 tool_call 都要被训练到。"""

import json
import sys

sys.path.insert(0, r"G:\PyArx\tools\train")

import train_lora_cpu as T
from transformers import AutoTokenizer

tok = AutoTokenizer.from_pretrained(T.MODEL_DIR, trust_remote_code=True)
rows = [json.loads(l) for l in open(T.DATA, encoding="utf-8") if l.strip()]

chain = [r for r in rows if any(m.get("role") == "tool" for m in r["messages"])]
print("多轮样本数:", len(chain))

row = chain[0]
full = T.render(row["messages"])
ids, labs = T.build_labels(tok, full, T.MAX_LEN)
loss_txt = tok.decode([i for i, l in zip(ids, labs) if l != -100])
print("tokens=%d loss_tokens=%d" % (len(ids), sum(1 for x in labs if x != -100)))
print("--- loss 段内容 ---")
print(loss_txt)
print("--- 含 assistant 标记? ---", "assistant" in loss_txt)
