# -*- coding: utf-8 -*-
"""与训练 render() 同构的验收：system 用训练数据原文，不塞工具清单。

训练时 assistant 目标是：
<tool_call>
{"name": "cad_line", "arguments": {"p1": [0, 0], "p2": [120, 0], "layer": "LIVE_TEST"}}
</tool_call>
只要 prompt 与训练同构，才能测出模型真实能力；塞工具清单属于分布外输入，
测出来的分数不能代表模型，只代表测法。
"""

import json
import os
import re
import sys

import torch

torch.set_num_threads(int(os.environ.get("CAD_TRAIN_THREADS", "4")))

BASE = os.environ.get("CAD_BASE_MODEL", r"D:\models\qwen2.5-1.5b")
ADAPTER = os.environ.get("CAD_ADAPTER_OUT", r"G:\PyArx\tools\train\out\qwen1.5b-cad-lora-api318")
DATA = os.environ.get("CAD_TRAIN_DATA", r"G:\PyArx\tools\train\data\sft_all_api.jsonl")

CASES = [
    ("在 LIVE_TEST 层从 (0,0) 到 (120,0) 画一条线",
     "cad_line", {"p1": [0, 0], "p2": [120, 0], "layer": "LIVE_TEST"}),
    ("画一个圆心在 60,40 半径 12 的圆",
     "cad_circle", {"center": [60, 40], "radius": 12}),
    ("在 0,150 位置写 8 号字「配电箱」",
     "cad_text", {"pos": [0, 150], "text": "配电箱", "height": 8}),
    ("统计一下当前图里有多少实体、分别在哪层",
     "cad_stats", {}),
    ("查一下 LIVE_TEST 层上都有什么",
     "cad_query", {"layer": "LIVE_TEST"}),
    ("把句柄 698 的那个图元删掉",
     "cad_erase", {"handles": ["698"]}),
]


def train_system():
    with open(DATA, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                return json.loads(line)["messages"][0]["content"]
    return ""


TC_RE = re.compile(r"<tool_call>\s*(\{.*?\})\s*</tool_call>", re.S)


def main():
    from transformers import AutoTokenizer, AutoModelForCausalLM
    from peft import PeftModel

    system = train_system()
    tok = AutoTokenizer.from_pretrained(BASE, trust_remote_code=True)
    if tok.pad_token is None:
        tok.pad_token = tok.eos_token
    model = AutoModelForCausalLM.from_pretrained(BASE, torch_dtype=torch.float32,
                                                 low_cpu_mem_usage=True,
                                                 trust_remote_code=True)
    if os.path.isdir(ADAPTER):
        model = PeftModel.from_pretrained(model, ADAPTER)
        print("use adapter: %s" % ADAPTER, flush=True)
    else:
        print("adapter missing, testing base model", flush=True)
    model.eval()

    hit = 0
    for utt, want_name, want_args in CASES:
        prompt = ("<|im_start|>system\n" + system + "<|im_end|>\n"
                  "<|im_start|>user\n" + utt + "<|im_end|>\n"
                  "<|im_start|>assistant\n")
        ids = tok(prompt, return_tensors="pt", add_special_tokens=False).input_ids
        with torch.no_grad():
            out = model.generate(ids, max_new_tokens=120, do_sample=False,
                                 pad_token_id=tok.pad_token_id or tok.eos_token_id)
        text = tok.decode(out[0][ids.shape[1]:], skip_special_tokens=True)

        got_name, got_args, raw = None, None, text.strip()
        m = TC_RE.search(text)
        if m:
            try:
                obj = json.loads(m.group(1))
                got_name = obj.get("name")
                got_args = obj.get("arguments")
            except Exception:
                pass
        ok = (got_name == want_name)
        hit += 1 if ok else 0
        print("\n[%s] %s" % ("HIT" if ok else "MISS", utt), flush=True)
        print("  want: %s %s" % (want_name, json.dumps(want_args, ensure_ascii=False)), flush=True)
        print("  got : %s %s" % (got_name, json.dumps(got_args, ensure_ascii=False)), flush=True)
        if not ok:
            print("  raw : %s" % raw[:200].replace("\n", " "), flush=True)

    print("\nname-match %d/%d" % (hit, len(CASES)), flush=True)


if __name__ == "__main__":
    main()
