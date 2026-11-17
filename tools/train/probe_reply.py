# -*- coding: utf-8 -*-
"""在 CPU 上加载 base + LoRA adapter，对固定用例生成回复，用于验证训练效果。

用法：
  C:\temp\py312\python.exe G:\PyArx\tools\train\probe_reply.py
可选环境变量：
  CAD_BASE_MODEL   base 模型目录（默认 D:\models\qwen2.5-1.5b）
  CAD_ADAPTER_OUT  adapter 目录；设为空串则只跑 base，做对照
  CAD_CPU_THREADS  CPU 线程数（默认 8）
  CAD_MAX_NEW      最多生成 token（默认 96）
"""
import os, json, time

import torch
from transformers import AutoTokenizer, AutoModelForCausalLM
try:
    from peft import PeftModel
except ImportError:
    PeftModel = None

BASE = os.environ.get("CAD_BASE_MODEL", r"D:\models\qwen2.5-1.5b")
ADAPTER = os.environ.get("CAD_ADAPTER_OUT", r"G:\PyArx\tools\train\out\qwen1.5b-cad-lora-api318")
DATA = os.environ.get("CAD_TRAIN_DATA", r"G:\PyArx\tools\train\data\sft_all_api.jsonl")
MAX_NEW = int(os.environ.get("CAD_MAX_NEW", "96"))

# system 与训练数据严格同口径，避免推理时口径漂移
with open(DATA, encoding="utf-8") as f:
    SYS = json.loads(f.readline())["messages"][0]["content"]

PROMPTS = [
    ("在 LIVE_TEST 层从 (0,0) 到 (120,0) 画一条线", "cad_line"),
    ("把句柄 698 的那个图元删掉", "cad_erase"),
    ("画一个 800 长 600 高的柜，横向三等分，第一格标题写主回路", "cad_rect"),
    ("在 0,0 插入配电箱符号块", "cad_block"),
    ("为什么图层在事务里建会失败？", None),
]

threads = int(os.environ.get("CAD_CPU_THREADS", "8"))
torch.set_num_threads(threads)

print("[probe] base = %s" % BASE, flush=True)
print("[probe] adapter = %s" % (ADAPTER or "(无, 只跑基座)"), flush=True)

tok = AutoTokenizer.from_pretrained(BASE, trust_remote_code=True)
model = AutoModelForCausalLM.from_pretrained(
    BASE, torch_dtype=torch.float32, low_cpu_mem_usage=True, trust_remote_code=True)

if ADAPTER and PeftModel and os.path.isfile(os.path.join(ADAPTER, "adapter_config.json")):
    model = PeftModel.from_pretrained(model, ADAPTER)
    print("[probe] adapter 已挂载", flush=True)
model.eval()

hit = 0
total = 0
for q, expect in PROMPTS:
    msgs = [{"role": "system", "content": SYS}, {"role": "user", "content": q}]
    text = tok.apply_chat_template(msgs, tokenize=False, add_generation_prompt=True)
    enc = tok(text, return_tensors="pt")
    t0 = time.time()
    with torch.no_grad():
        out = model.generate(**enc, max_new_tokens=MAX_NEW, do_sample=False,
                             pad_token_id=tok.pad_token_id, eos_token_id=tok.eos_token_id)
    ans = tok.decode(out[0][enc["input_ids"].shape[1]:], skip_special_tokens=True).strip()
    took = time.time() - t0
    if expect:
        total += 1
        ok = expect in ans
        hit += 1 if ok else 0
    else:
        ok = None
    print("\n================ Q ================", flush=True)
    print(q, flush=True)
    print("================ A ================", flush=True)
    print(ans[:500], flush=True)
    print("-- 耗时 %.1fs  期望工具 %s  命中 %s" % (took, expect, ok), flush=True)

print("\n[probe] 工具命中 %d/%d" % (hit, total), flush=True)
