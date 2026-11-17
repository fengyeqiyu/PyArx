# -*- coding: utf-8 -*-
"""6 例 eval 的原始生成留档：逐条统计 wrapped tool_call 数、坏词、参数比对。"""
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

SYSTEM = (
    "你是 CAD 绘图助手。只通过 cad_* 工具操作图形，不要自己写 PyArx 代码。"
    "用户要脚本或代码时照样输出 cad_* 工具调用，由系统生成 PyArx 代码，不要改成讲架构或手写代码。"
    "坐标单位 mm，角度用度，图层不存在会自动创建。"
    "cad_layer 只用于新建图层；要查某层上已有哪些图元用 cad_query。"
    "一次事务全有或全无；工具返回 ok/count/handles/warnings/errors，"
    "失败时要读 warnings 自纠而不是重试同一调用。"
)
tool_desc = "\n".join("- %s: %s" % (s["name"], s["description"].split("。")[0])
                      for s in cad_ops.SCHEMAS)
TC_RE = re.compile(r"<tool_call>\s*(\{.*?\})\s*</tool_call>", re.S)

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

for utt, want_name, want_args in CASES:
    prompt = ("<|im_start|>system\n%s\n可用工具：\n%s<|im_end|>\n"
              "<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n"
              % (SYSTEM, tool_desc, utt))
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
    got_name = (tc or {}).get("name")
    got_args = (tc or {}).get("arguments")
    bad = any(w in raw for w in ("cad_entity", "assistant"))
    print("=== %s" % utt)
    print("elapsed=%.2fs  call_count=%d  name_ok=%s  args_exact=%s  bad_token=%s"
          % (dt, len(matches), got_name == want_name, got_args == want_args, bad))
    print("want: %s %s" % (want_name, json.dumps(want_args, ensure_ascii=False)))
    print("got : %s %s" % (got_name, json.dumps(got_args, ensure_ascii=False)))
    print("RAW: %s" % raw.replace("\n", " "))
    print()
