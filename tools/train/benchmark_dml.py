# -*- coding: utf-8 -*-
"""RX580 8G 上 DirectML 训练吞吐基准，与同环境 CPU 对跑。

只测 forward+backward+step 的稳态步耗时，不接数据集、不接 tokenizer，
避免把数据管线的时间混进来。用法：
    python benchmark_dml.py --cpu
    python benchmark_dml.py --dml
加 --lora 走真实训练路径（冻结 base，只优化 adapter）。

2026-11-16 实测（RX580 2048SP 8G / torch-directml 0.2.7 / torch 2.4.1）：
    LoRA bs=1 seq=64   cpu 0.81s   dml 1.33s   -> GPU 慢 64%
    LoRA bs=4 seq=128  cpu 3.58s   dml 2.54s   -> GPU 快 41%
结论：只有大 batch 才划算；小 batch 时 AdamW 的 aten::lerp.Scalar_out
不支持 DML、回退 CPU，权重要来回搬，反而更慢。
"""

import os
import sys
import time

MODEL = os.environ.get("BENCH_MODEL", r"D:\models\qwen2.5-0.5b")
DTYPE_NAME = os.environ.get("BENCH_DTYPE", "float32")
BS = int(os.environ.get("BENCH_BS", 4))
SEQ = int(os.environ.get("BENCH_SEQ", 128))
STEPS = int(os.environ.get("BENCH_STEPS", 5))


def bench(device, tag, use_lora):
    import torch
    from transformers import AutoModelForCausalLM

    print("[%s] loading ..." % tag, flush=True)
    t0 = time.time()
    torch_dtype = torch.float16 if DTYPE_NAME == "float16" else torch.float32
    try:
        m = AutoModelForCausalLM.from_pretrained(
            MODEL, dtype=torch_dtype, attn_implementation="eager")
    except TypeError:
        m = AutoModelForCausalLM.from_pretrained(
            MODEL, torch_dtype=torch_dtype, attn_implementation="eager")

    if use_lora:
        from peft import LoraConfig, get_peft_model
        cfg = LoraConfig(r=8, lora_alpha=16, lora_dropout=0.0, bias="none",
                         target_modules=["q_proj", "k_proj", "v_proj", "o_proj"],
                         task_type="CAUSAL_LM")
        m = get_peft_model(m, cfg)
        m.print_trainable_parameters()

    m = m.to(device)
    m.train()
    print("[%s] loaded in %.1fs" % (tag, time.time() - t0), flush=True)

    params = [p for p in m.parameters() if p.requires_grad]
    opt = torch.optim.AdamW(params, lr=1e-4)
    ids = torch.randint(0, 1000, (BS, SEQ)).to(device)
    labs = torch.randint(0, 1000, (BS, SEQ)).to(device)

    # 预热一步：DirectML 首次执行要编译算子
    out = m(input_ids=ids, labels=labs)
    out.loss.backward()
    opt.step()
    opt.zero_grad()
    print("[%s] warmup done" % tag, flush=True)

    t0 = time.time()
    for i in range(STEPS):
        out = m(input_ids=ids, labels=labs)
        out.loss.backward()
        opt.step()
        opt.zero_grad()
    dt = time.time() - t0
    return dt / STEPS, float(out.loss.detach())


def main():
    tag = "cpu"
    device = "cpu"
    use_lora = "--lora" in sys.argv
    if "--dml" in sys.argv:
        import torch_directml

        if torch_directml.device_count() == 0:
            print("RESULT no directml device")
            return
        device = torch_directml.device()
        tag = "directml"
    if use_lora:
        tag = tag + "+lora"
    try:
        sec, loss = bench(device, tag, use_lora)
        print("RESULT %s step=%.2fs loss=%.4f device=%s bs=%d seq=%d"
              % (tag, sec, loss, device, BS, SEQ))
    except Exception as exc:
        print("RESULT %s FAILED %r" % (tag, exc))


main()
