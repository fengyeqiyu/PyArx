# -*- coding: utf-8 -*-
"""DirectML 上 LoRA 微调 Qwen2.5-0.5B（RX580 8G 专用）。

与 train_lora_cpu.py 同一份数据、同一套 LoRA 超参、同一个 build_labels，
差异只有四点：
  1. 模型与每个 batch 都在 DirectML 设备上；
  2. MAX_LEN 默认 384 —— 实测本数据集 max=403、98.6% 的样本 ≤384，
     而 DirectML 的可用预算撑不住 640（bs=1/seq=512 实测 OOM，上限就是 384）；
  3. batch 默认 1，用梯度累积补有效 batch（实测 bs=2/seq=384 也会 OOM）；
  4. DataLoader num_workers=0，避免多进程与 DML 抢设备。

已知代价：AdamW 的 aten::lerp.Scalar_out 不支持 DML，会回退 CPU 执行，
每步有额外搬运开销，所以小 batch 下不一定比 CPU 快。
"""

import json
import os
import time

import torch
from torch.utils.data import DataLoader, Dataset

torch.set_num_threads(int(os.environ.get("CAD_TRAIN_THREADS", "4")))

MODEL_DIR = os.environ.get("CAD_BASE_MODEL", r"D:\models\qwen2.5-0.5b")
DATA = os.environ.get("CAD_TRAIN_DATA", r"G:\PyArx\tools\train\data\sft_all.jsonl")
OUT = os.environ.get("CAD_ADAPTER_OUT", r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-dml")
MAX_LEN = int(os.environ.get("CAD_TRAIN_MAXLEN", "384"))
EPOCHS = int(os.environ.get("CAD_TRAIN_EPOCHS", "2"))
BATCH = int(os.environ.get("CAD_TRAIN_BATCH", "1"))
LR = float(os.environ.get("CAD_TRAIN_LR", "1e-4"))
ACCUM = int(os.environ.get("CAD_TRAIN_ACCUM", "4"))
MAX_STEPS = int(os.environ.get("CAD_TRAIN_MAX_STEPS", "0"))
# 每 SAVE_EVERY 个微步存一份 LoRA 断点（只存 lora_ 权重，CPU 张量），
# 全量 300k 步要跑十几天，没有中途断点一旦掉电/崩溃全部作废。
SAVE_EVERY = int(os.environ.get("CAD_TRAIN_SAVE_EVERY", "2000"))
SEED = int(os.environ.get("CAD_TRAIN_SEED", "1234"))
# 每 SAMPLE_EVERY 步用「当前正在训练的权重」直接生成几条固定用例，
# 把问答写进日志，界面上就能看到模型这一步到底会说什么（0=关闭）。
SAMPLE_EVERY = int(os.environ.get("CAD_TRAIN_SAMPLE", "0"))
SAMPLE_PROMPTS = [
    "在 LIVE_TEST 层从 (0,0) 到 (120,0) 画一条线",
    "把句柄 698 的那个图元删掉",
    "画一个 800 长 600 高的柜，横向三等分，第一格标题写主回路",
]


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
                # 一次可以吐多个调用（联动样本），不能只取第一个
                body = "\n".join(
                    '<tool_call>\n{"name": "%s", "arguments": %s}\n</tool_call>'
                    % (fn["name"], fn["arguments"]) for fn in m["tool_calls"])
            else:
                body = m.get("content") or ""
            parts.append("<|im_start|>assistant\n" + body + "<|im_end|>\n")
    return "".join(parts)


def build_labels(tok, full, max_len):
    """所有 assistant 段都算 loss，`<|im_start|>assistant` 标记本身不算。

    多轮样本里每一步都是模型的动作，只训最后一步的话"先查再动手"学不到。
    标记本身不能算 loss：推理时模板已经给出该标记，当目标会让模型自己吐 assistant。
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
            # im_end 必须计入 loss，否则模型没学过"何时收尾"
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
        row = self.rows[i]
        msgs = row["messages"]
        full = render(msgs)
        ids, labels = build_labels(self.tok, full, MAX_LEN)
        if not ids:
            ids, labels = [self.tok.eos_token_id], [-100]
        # 带上样本身份，训练循环会把「这一步到底在学什么」写进日志
        meta = row.get("meta") or {}
        prompt = ""
        for m in msgs:
            if m.get("role") == "user":
                prompt = (m.get("content") or "").replace("\n", " ").strip()
                break
        if len(prompt) > 60:
            prompt = prompt[:60] + "…"
        return {"input_ids": ids, "labels": labels,
                "kind": str(meta.get("kind") or ""),
                "tool": str(meta.get("tool") or ""),
                "prompt": prompt}


def collate(batch, pad_id):
    mx = max(len(b["input_ids"]) for b in batch)
    ids, labs, att = [], [], []
    for b in batch:
        n = mx - len(b["input_ids"])
        ids.append(b["input_ids"] + [pad_id] * n)
        labs.append(b["labels"] + [-100] * n)
        att.append([1] * len(b["input_ids"]) + [0] * n)
    metas = [(b.get("kind", ""), b.get("tool", ""), b.get("prompt", "")) for b in batch]
    return (torch.tensor(ids), torch.tensor(att), torch.tensor(labs), metas)


def _lora_state(model):
    # 只挑 LoRA 权重，全部搬回 CPU 再存：DirectML 张量是 OpaqueTensorImpl，
    # torch.save 直接吃会 NotImplementedError。
    sd = {}
    for k, v in model.state_dict().items():
        if "lora_" in k:
            sd[k] = v.detach().to("cpu").clone()
    return sd


def save_ckpt(model, out_dir, step, rows, epochs):
    os.makedirs(out_dir, exist_ok=True)
    fn = "ckpt_step%d.pt" % step
    torch.save(_lora_state(model), os.path.join(out_dir, fn))
    tmp = os.path.join(out_dir, "ckpt_state.json.tmp")
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump({"step": step, "file": fn, "rows": rows, "epochs": epochs}, f)
    os.replace(tmp, os.path.join(out_dir, "ckpt_state.json"))
    return fn


def load_ckpt(model, out_dir):
    """返回 (已完成微步数, 断点记录的样本数)；没有可用断点时返回 (0, -1)。"""
    state_p = os.path.join(out_dir, "ckpt_state.json")
    if not os.path.exists(state_p):
        return 0, -1
    try:
        with open(state_p, encoding="utf-8") as f:
            st = json.load(f)
        p = os.path.join(out_dir, st.get("file", ""))
        if not os.path.exists(p):
            return 0, -1
        sd = torch.load(p, map_location="cpu")
        missing, unexpected = model.load_state_dict(sd, strict=False)
        print("[3/4] resume <- %s step=%d (miss=%d unexp=%d)"
              % (p, int(st.get("step", 0)), len(missing), len(unexpected)), flush=True)
        return int(st.get("step", 0)), int(st.get("rows", -1))
    except Exception as e:
        print("[3/4] resume failed: %r" % (e,), flush=True)
        return 0, -1


def main():
    import torch_directml
    from transformers import AutoTokenizer, AutoModelForCausalLM
    from peft import LoraConfig, get_peft_model, TaskType

    dev = torch_directml.device()
    print("[0/4] device: %s" % torch_directml.device_name(0), flush=True)

    _dt = torch.float16 if os.environ.get("CAD_TRAIN_DTYPE", "fp16") == "fp16" else torch.float32
    print("[1/4] load base: %s  dtype=%s" % (MODEL_DIR, _dt), flush=True)
    tok = AutoTokenizer.from_pretrained(MODEL_DIR, trust_remote_code=True)
    if tok.pad_token is None:
        tok.pad_token = tok.eos_token
    model = AutoModelForCausalLM.from_pretrained(MODEL_DIR, torch_dtype=_dt,
                                                 low_cpu_mem_usage=True,
                                                 trust_remote_code=True)
    model.config.use_cache = False

    print("[2/4] attach lora", flush=True)
    cfg = LoraConfig(task_type=TaskType.CAUSAL_LM, r=8, lora_alpha=16, lora_dropout=0.05,
                     target_modules=["q_proj", "k_proj", "v_proj", "o_proj",
                                     "gate_proj", "up_proj", "down_proj"])
    model = get_peft_model(model, cfg)
    if os.environ.get("CAD_TRAIN_CKPT", "1") == "1":
        # 384 长度下 fp16 权重的 logits 会撞到 6.5GB 上限（实测 cross_entropy 缺 698MB），
        # 用重算换显存：fwd 1.7s / bwd 2.8s，仍比 CPU 快一倍以上。
        model.gradient_checkpointing_enable()
        model.enable_input_require_grads()
        print("[2/4] gradient checkpointing ON", flush=True)
    model.print_trainable_parameters()
    resume_step, resume_rows = load_ckpt(model, OUT)
    model = model.to(dev)

    ds = SFT(DATA, tok)
    if resume_step > 0 and resume_rows != len(ds):
        print("[3/4] ckpt rows=%d 与数据集 rows=%d 不一致，忽略断点从头训"
              % (resume_rows, len(ds)), flush=True)
        resume_step = 0
    # 抽样用的 system 与训练数据同口径，否则推理时会因为系统提示不同而跑偏
    sys_prompt = ""
    with open(DATA, encoding="utf-8") as f:
        first = f.readline().strip()
        if first:
            for m in json.loads(first)["messages"]:
                if m.get("role") == "system":
                    sys_prompt = m.get("content") or ""
                    break
    torch.manual_seed(SEED)  # 断点续训要靠固定 seed 保证 shuffle 顺序一致
    dl = DataLoader(ds, batch_size=BATCH, shuffle=True,
                    collate_fn=lambda b: collate(b, tok.pad_token_id))
    print("[3/4] train: rows=%d epochs=%d batch=%d accum=%d maxlen=%d resume=%d save_every=%d"
          % (len(ds), EPOCHS, BATCH, ACCUM, MAX_LEN, resume_step, SAVE_EVERY), flush=True)

    opt = torch.optim.AdamW([p for p in model.parameters() if p.requires_grad], lr=LR)
    model.train()
    t0 = time.time()
    step = 0
    if resume_step > 0:
        print("[3/4] skip %d / %d 已完成微步" % (resume_step, len(ds) * EPOCHS), flush=True)
    for ep in range(EPOCHS):
        for ids, att, labs, metas in dl:
            if step < resume_step:
                step += 1
                continue
            ids = ids.to(dev)
            att = att.to(dev)
            labs = labs.to(dev)
            out = model(input_ids=ids, attention_mask=att, labels=labs)
            loss = out.loss / ACCUM
            loss.backward()
            step += 1
            if step % ACCUM == 0:
                opt.step()
                opt.zero_grad()
            kind, tool, prompt = metas[0]
            tag = (kind + ("/" + tool if tool else "")) if kind else "-"
            print("  ep%d step%d loss=%.4f  %.0fs  | %s | %s"
                  % (ep + 1, step, loss.item() * ACCUM, time.time() - t0, tag, prompt),
                  flush=True)
            # 周期断点：只存 LoRA 权重，异常只跳过保存，绝不打断训练。
            if SAVE_EVERY > 0 and step % SAVE_EVERY == 0:
                try:
                    fn = save_ckpt(model, OUT, step, len(ds), EPOCHS)
                    print("  >> ckpt %s  %.1f MB" % (fn, os.path.getsize(os.path.join(OUT, fn)) / 1e6),
                          flush=True)
                except Exception as e:
                    print("  >> ckpt failed step%d: %r" % (step, e), flush=True)
            # 用显存里当前权重直接生成，不加载第二份模型，不会 OOM；
            # 任何异常只跳过抽样，绝不打断训练。
            if SAMPLE_EVERY > 0 and step % SAMPLE_EVERY == 0:
                try:
                    model.eval()
                    model.config.use_cache = True
                    for q in SAMPLE_PROMPTS:
                        text = render([{"role": "system", "content": sys_prompt},
                                       {"role": "user", "content": q}])
                        text += "<|im_start|>assistant\n"
                        enc = tok(text, return_tensors="pt").to(dev)
                        with torch.no_grad():
                            gen = model.generate(**enc, max_new_tokens=64, do_sample=False,
                                                 pad_token_id=tok.pad_token_id,
                                                 eos_token_id=tok.eos_token_id)
                        ans = tok.decode(gen[0][enc["input_ids"].shape[1]:],
                                         skip_special_tokens=True).strip()
                        ans = ans.replace("\r", " ").replace("\n", " ")
                        print("  >> sample@%d | %s | %s" % (step, q, ans[:180]), flush=True)
                except Exception as e:
                    print("  >> sample failed step%d: %r" % (step, e), flush=True)
                finally:
                    model.config.use_cache = False
                    model.train()
            if MAX_STEPS > 0 and step >= MAX_STEPS:
                break
        if MAX_STEPS > 0 and step >= MAX_STEPS:
            break
        print("epoch %d done  %.0fs" % (ep + 1, time.time() - t0), flush=True)
        try:
            fn = save_ckpt(model, OUT, step, len(ds), EPOCHS)
            print("  >> ckpt(epoch) %s" % fn, flush=True)
        except Exception as e:
            print("  >> ckpt(epoch) failed: %r" % (e,), flush=True)

    print("[4/4] save adapter -> %s" % OUT, flush=True)
    # peft 的 save_pretrained 要读每个张量的 storage，而 DirectML 张量是
    # OpaqueTensorImpl，直接存会 NotImplementedError，所以先整体搬回 CPU。
    model = model.to("cpu")
    os.makedirs(OUT, exist_ok=True)
    model.save_pretrained(OUT)
    tok.save_pretrained(OUT)
    print("done %.0fs" % (time.time() - t0), flush=True)


if __name__ == "__main__":
    main()
