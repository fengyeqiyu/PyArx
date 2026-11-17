# -*- coding: utf-8 -*-
"""0.5B/1.5B 收口验证：两条关键用例，检查单 tool_call、正常收尾、参数、耗时。"""
import json, os, re, sys, time, torch
torch.set_num_threads(int(os.environ.get("CAD_TRAIN_THREADS", "6")))

BASE = os.environ.get("CAD_BASE_MODEL", r"D:\models\qwen2.5-0.5b")
ADAPTER = os.environ.get("CAD_ADAPTER_OUT", r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora")
sys.path.insert(0, r"G:\PyArx\tools\cadmcp")
import cad_ops

from transformers import AutoTokenizer, AutoModelForCausalLM
from peft import PeftModel

tok = AutoTokenizer.from_pretrained(BASE, trust_remote_code=True)
model = AutoModelForCausalLM.from_pretrained(BASE, torch_dtype=torch.float32, trust_remote_code=True)
if os.path.isdir(ADAPTER):
    model = PeftModel.from_pretrained(model, ADAPTER)
model.eval()

SYS = ("你是 CAD 绘图助手。只通过 cad_* 工具操作图形，不要自己写 PyArx 代码。"
       "坐标单位 mm，角度用度，图层不存在会自动创建。"
       "cad_layer 只用于新建图层；要查某层上已有哪些图元用 cad_query。"
       "用户要脚本或代码时照样输出 cad_* 工具调用，由系统生成 PyArx 代码，"
       "不要改成讲架构或手写代码。"
       "一次事务全有或全无；工具返回 ok/count/handles/warnings/errors，"
       "失败时要读 warnings 自纠而不是重试同一调用。")
tool_desc = "\n".join("- %s: %s" % (s["name"], s["description"].split("。")[0])
                      for s in cad_ops.SCHEMAS)
TC_RE = re.compile(r"<tool_call>\s*(\{.*?\})\s*</tool_call>", re.S)

CASES = [
    ("在原点画一条到 100.0 的线",
     "cad_line", {"p1": [0, 0], "p2": [100, 0]}),
    ("写一段 Python 脚本，在原点画一条到 100 的线",
     "cad_line", {"p1": [0, 0], "p2": [100, 0]}),
]

for utt, want_name, want_args in CASES:
    prompt = ("<|im_start|>system\n%s\n可用工具：\n%s<|im_end|>\n"
              "<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n"
              % (SYS, tool_desc, utt))
    ids = tok(prompt, return_tensors="pt").input_ids
    t0 = time.time()
    with torch.no_grad():
        out = model.generate(ids, max_new_tokens=120, do_sample=False,
                             pad_token_id=tok.pad_token_id or tok.eos_token_id)
    dt = time.time() - t0
    raw = tok.decode(out[0][ids.shape[1]:], skip_special_tokens=True).strip()
    matches = TC_RE.findall(raw)
    tc = None
    if matches:
        try:
            tc = json.loads(matches[0])
        except Exception as e:
            tc = {"_parse_error": str(e)}
    ok_name = tc is not None and tc.get("name") == want_name
    got_args = (tc or {}).get("arguments")
    ok_args = ok_name and got_args == want_args
    bad_word = any(w in raw for w in ("cad_entity", "assistant", "别写", "手写", "脚本架构"))
    print("=== %s" % utt)
    print("elapsed=%.2fs  tool_call_count=%d  name_ok=%s  args_ok=%s  bad_token=%s"
          % (dt, len(matches), ok_name, ok_args, bad_word))
    print("want: %s %s" % (want_name, json.dumps(want_args, ensure_ascii=False)))
    print("got : %s %s" % ((tc or {}).get("name"), json.dumps(got_args, ensure_ascii=False)))
    print("RAW: %s" % raw.replace("\n", " "))
    print()
