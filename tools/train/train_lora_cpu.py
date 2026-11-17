# -*- coding: utf-8 -*-
"""CPU 上 QLoRA/LoRA 微调 Qwen2.5-0.5B，学"指令 → cad_* 工具调用"。

设计要点：
  * 只在 assistant（工具调用）段算 loss，prompt 段 mask 成 -100 —— 样本只有几百条，
    全序列训练会把大量容量花在复读 system prompt 上。
  * 工具调用按 Qwen 约定的 <tool_call>{"name":...,"arguments":{...}}</tool_call> 输出，
    推理时正则抽出来再 json.loads，直接喂 server.call_cad。
  * torch.set_num_threads(6)：留 2 核给系统/CAD，避免整机卡死。
"""

import json
import os
import sys
import time

import torch
from torch.utils.data import DataLoader, Dataset

torch.set_num_threads(int(os.environ.get("CAD_TRAIN_THREADS", "6")))

MODEL_DIR = os.environ.get("CAD_BASE_MODEL", r"D:\models\qwen2.5-0.5b")
DATA = r"G:\PyArx\tools\train\data\sft_all.jsonl"
OUT = os.environ.get("CAD_ADAPTER_OUT", r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora")
MAX_LEN = int(os.environ.get("CAD_TRAIN_MAXLEN", "640"))
EPOCHS = int(os.environ.get("CAD_TRAIN_EPOCHS", "3"))
BATCH = int(os.environ.get("CAD_TRAIN_BATCH", "4"))
LR = float(os.environ.get("CAD_TRAIN_LR", "1e-4"))
MAX_STEPS = int(os.environ.get("CAD_TRAIN_MAX_STEPS", "0"))


def render(msgs):
    parts = []
    for m in msgs:
        role = m.get("role")
        if role == "system":
            parts.append("<|im_start|>system\n" + (m.get("content") or "") + "<|im_end|>\n")
        elif role == "user":
            parts.append("<|im_start|>user\n" + (m.get("content") or "") + "<|im_end|>\n")
        elif role == "tool":
            parts.append("<|im_start|>tool\n" + (m.get("content") or "") + "<|im_end|>\n")
        elif role == "assistant":
            if m.get("tool_calls"):
                fn = m["tool_calls"][0]["function"]
                body = ('<tool_call>\n{"name": "%s", "arguments": %s}\n</tool_call>'
                        % (fn["name"], fn["arguments"]))
            else:
                body = m.get("content") or ""
            parts.append("<|im_start|>assistant\n" + body + "<|im_end|>\n")
    return "".join(parts)


def build_labels(tok, full, max_len):
    """所有 assistant 段都算 loss，`<|im_start|>assistant` 标记本身不算。

    多轮样本（query -> 拿到句柄 -> 继续操作）里每一步都是模型的动作，
    只训最后一步的话，"先查再动手"这个最该学的决策反而学不到。
    标记本身也不能算 loss：推理时模板已经给出该标记，再当成目标会让模型自己吐 "assistant"。
    """
    marker = "<|im_start|>assistant\n"
    ender = "<|im_end|>"
    ids, labels = [], []
    i, n = 0, len(full)
    while i < n:
        m = full.find(marker, i)
        if m < 0:
            sid = tok(full[i:], add_special_tokens=False)["input_ids"]
            ids += sid
            labels += [-100] * len(sid)
            break
        if m > i:
            sid = tok(full[i:m], add_special_tokens=False)["input_ids"]
            ids += sid
            labels += [-100] * len(sid)
        hid = tok(marker, add_special_tokens=False)["input_ids"]
        ids += hid
        labels += [-100] * len(hid)
        e = full.find(ender, m + len(marker))
        body = full[m + len(marker):(e if e > 0 else n)]
        bid = tok(body, add_special_tokens=False)["input_ids"]
        ids += bid
        labels += bid
        if e > 0:
            # im_end 必须计入 loss：否则模型没学过"何时收尾"，生成完一个 tool_call
            # 会继续自回归编第二、第三个，实测一次吐 5 个才撞上限，单轮推理从 7 秒涨到 33 秒。
            eid = tok(ender, add_special_tokens=False)["input_ids"]
            ids += eid
            labels += eid
            i = e + len(ender)
        else:
            i = n
    return ids[:max_len], labels[:max_len]


class SFT(Dataset):
    def __init__(self, path, tok):
        self.rows = []
        with open(path, encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line:
                    self.rows.append(json.loads(line))
        self.tok = tok

    def __len__(self):
        return len(self.rows)

    def __getitem__(self, i):
        msgs = self.rows[i]["messages"]
        full = render(msgs)
        ids, labels = build_labels(self.tok, full, MAX_LEN)
        if not ids:
            ids, labels = [self.tok.eos_token_id], [-100]
        return {"input_ids": ids, "labels": labels}


def collate(batch, pad_id):
    mx = max(len(b["input_ids"]) for b in batch)
    ids, labs, att = [], [], []
    for b in batch:
        n = mx - len(b["input_ids"])
        ids.append(b["input_ids"] + [pad_id] * n)
        labs.append(b["labels"] + [-100] * n)
        att.append([1] * len(b["input_ids"]) + [0] * n)
    return (torch.tensor(ids), torch.tensor(att), torch.tensor(labs))


def main():
    from transformers import AutoTokenizer, AutoModelForCausalLM
    from peft import LoraConfig, get_peft_model, TaskType

    print("[1/4] load base: %s" % MODEL_DIR, flush=True)
    tok = AutoTokenizer.from_pretrained(MODEL_DIR, trust_remote_code=True)
    if tok.pad_token is None:
        tok.pad_token = tok.eos_token
    model = AutoModelForCausalLM.from_pretrained(MODEL_DIR, torch_dtype=torch.float32,
                                                 trust_remote_code=True)
    model.config.use_cache = False

    print("[2/4] attach lora", flush=True)
    cfg = LoraConfig(task_type=TaskType.CAUSAL_LM, r=8, lora_alpha=16, lora_dropout=0.05,
                     target_modules=["q_proj", "k_proj", "v_proj", "o_proj",
                                     "gate_proj", "up_proj", "down_proj"])
    model = get_peft_model(model, cfg)
    model.print_trainable_parameters()

    ds = SFT(DATA, tok)
    dl = DataLoader(ds, batch_size=BATCH, shuffle=True,
                    collate_fn=lambda b: collate(b, tok.pad_token_id))
    print("[3/4] train: rows=%d epochs=%d batch=%d threads=%d"
          % (len(ds), EPOCHS, BATCH, torch.get_num_threads()), flush=True)

    opt = torch.optim.AdamW([p for p in model.parameters() if p.requires_grad], lr=LR)
    model.train()
    t0 = time.time()
    step = 0
    for ep in range(EPOCHS):
        for ids, att, labs in dl:
            out = model(input_ids=ids, attention_mask=att, labels=labs)
            loss = out.loss
            loss.backward()
            opt.step()
            opt.zero_grad()
            step += 1
            print("  ep%d step%d loss=%.4f  %.0fs"
                  % (ep + 1, step, loss.item(), time.time() - t0), flush=True)
            if MAX_STEPS > 0 and step >= MAX_STEPS:
                break
        if MAX_STEPS > 0 and step >= MAX_STEPS:
            break
        print("epoch %d done  %.0fs" % (ep + 1, time.time() - t0), flush=True)

    print("[4/4] save adapter -> %s" % OUT, flush=True)
    os.makedirs(OUT, exist_ok=True)
    model.save_pretrained(OUT)
    tok.save_pretrained(OUT)
    print("done %.0fs" % (time.time() - t0), flush=True)


if __name__ == "__main__":
    main()
