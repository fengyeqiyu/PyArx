# -*- coding: utf-8 -*-
import json, os, re, sys, torch
torch.set_num_threads(6)

BASE = r"D:\models\qwen2.5-0.5b"
ADAPTER = r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora"
sys.path.insert(0, r"G:\PyArx\tools\cadmcp")
import cad_ops

from transformers import AutoTokenizer, AutoModelForCausalLM
from peft import PeftModel

tok = AutoTokenizer.from_pretrained(BASE, trust_remote_code=True)
model = AutoModelForCausalLM.from_pretrained(BASE, torch_dtype=torch.float32, trust_remote_code=True)
model = PeftModel.from_pretrained(model, ADAPTER)
model.eval()

SYS = ("你是 CAD 绘图助手。只通过 cad_* 工具操作图形，不要自己写 PyArx 代码。"
       "坐标单位 mm，角度用度，图层不存在会自动创建。"
       "cad_layer 只用于新建图层；要查某层上已有哪些图元用 cad_query。"
       "一次事务全有或全无；工具返回 ok/count/handles/warnings/errors，"
       "失败时要读 warnings 自纠而不是重试同一调用。")
tool_desc = "\n".join("- %s: %s" % (s["name"], s["description"].split("。")[0])
                      for s in cad_ops.SCHEMAS)
TC_RE = re.compile(r"<tool_call>\s*(\{.*?\})\s*</tool_call>", re.S)

cases = [
    "在 LIVE_TEST 层从 (0,0) 到 (120,0) 画一条线",
    "把句柄 698 的那个图元删掉",
    "查一下 LIVE_TEST 层上都有什么",
]
for utt in cases:
    prompt = ("<|im_start|>system\n%s\n可用工具：\n%s<|im_end|>\n"
              "<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n"
              % (SYS, tool_desc, utt))
    ids = tok(prompt, return_tensors="pt").input_ids
    with torch.no_grad():
        out = model.generate(ids, max_new_tokens=120, do_sample=False,
                             pad_token_id=tok.pad_token_id or tok.eos_token_id)
    raw = tok.decode(out[0][ids.shape[1]:], skip_special_tokens=True).strip()
    m = TC_RE.search(raw)
    tc = json.loads(m.group(1)) if m else None
    print("=== %s" % utt)
    print("RAW: %s" % raw)
    print("PARSED: %s" % json.dumps(tc, ensure_ascii=False))
    print("contains 'assistant'? %s" % ("assistant" in raw))
    print()
