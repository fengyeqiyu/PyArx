# -*- coding: utf-8 -*-
"""模块问答 5 条：纯模型生成，按仓库真实 schema/事实关键词判分，不走抽取直答。"""
import json, os, sys, time, torch
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
       "一次事务全有或全无；工具返回 ok/count/handles/warnings/errors，"
       "失败时要读 warnings 自纠而不是重试同一调用。")
tool_desc = "\n".join("- %s: %s" % (s["name"], s["description"].split("。")[0])
                      for s in cad_ops.SCHEMAS)

# (问题, 必含关键词组：每组任一命中即可；全部组都命中才算答对)
CASES = [
    ("cad_offset 是干什么的？需要哪些关键参数？",
     [["偏移", "offset"], ["handle", "句柄"], ["dist", "距离"]]),
    ("cad_rect 画矩形需要给哪两个点？要不要自己算四个角？",
     [["p1"], ["p2"], ["不用", "不用自己", "工具层", "算"]]),
    ("要删除一张图里某个已有实体，正确的工具调用顺序是什么？",
     [["cad_query", "查询"], ["cad_erase", "删除"], ["handle", "句柄"]]),
    ("cad_query 按图层过滤用哪个参数？返回结果里能拿到什么供后续操作？",
     [["layer"], ["handle", "句柄"]]),
    ("cad_erase 清空整张图需要同时给哪两个参数，防止误清？",
     [["all"], ["ERASE_ALL"]]),
]

hit = 0
for q, groups in CASES:
    prompt = ("<|im_start|>system\n%s\n可用工具：\n%s<|im_end|>\n"
              "<|im_start|>user\n%s<|im_end|>\n<|im_start|>assistant\n"
              % (SYS, tool_desc, q))
    ids = tok(prompt, return_tensors="pt").input_ids
    t0 = time.time()
    with torch.no_grad():
        out = model.generate(ids, max_new_tokens=160, do_sample=False,
                             pad_token_id=tok.pad_token_id or tok.eos_token_id)
    dt = time.time() - t0
    raw = tok.decode(out[0][ids.shape[1]:], skip_special_tokens=True).strip()
    groups_hit = []
    for g in groups:
        groups_hit.append(any(k in raw for k in g))
    ok = all(groups_hit)
    hit += 1 if ok else 0
    print("=== %s" % q)
    print("elapsed=%.2fs  ok=%s  group_hit=%s" % (dt, ok, groups_hit))
    print("RAW: %s" % raw.replace("\n", " "))
    print()

print("module-qa %d/%d" % (hit, len(CASES)))
