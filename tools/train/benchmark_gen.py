# -*- coding: utf-8 -*-
"""真实生成吞吐：prefill + 逐 token 解码，CPU 与 DirectML 对跑。

    python benchmark_gen.py --cpu
    python benchmark_gen.py --dml
环境变量 BENCH_MODEL / BENCH_MAXNEW / BENCH_DTYPE 可覆盖。

2026-11-16 实测（Qwen2.5-1.5B / RX580 2048SP 8G / 32 prompt + 48 new）：
    cpu  fp32  21.19s  0.442s/tok
    dml  fp16  17.92s  0.373s/tok   -> 1.18x
    dml  fp32  14.38s  0.300s/tok   -> 1.47x
注意 fp16 反而比 fp32 慢：Polaris(GCN4) 的 fp16 不是双倍速率，
还要额外做 dtype 转换。要试就试 fp32。
"""

import os
import sys
import time

MODEL = os.environ.get("BENCH_MODEL", r"D:\models\qwen2.5-1.5b")
MAXNEW = int(os.environ.get("BENCH_MAXNEW", 48))
PROMPT_LEN = int(os.environ.get("BENCH_PROMPT", 32))
DTYPE = os.environ.get("BENCH_DTYPE", "float32")

TAG = "cpu"
DEVICE = "cpu"
if "--dml" in sys.argv:
    import torch_directml

    if torch_directml.device_count() == 0:
        print("RESULT no directml device")
        sys.exit(0)
    DEVICE = torch_directml.device()
    TAG = "directml"


def main():
    import torch
    from transformers import AutoModelForCausalLM

    torch_dtype = torch.float16 if DTYPE == "float16" else torch.float32
    name = os.path.basename(MODEL)
    m = AutoModelForCausalLM.from_pretrained(MODEL, torch_dtype=torch_dtype)
    m = m.to(DEVICE)
    m.eval()
    ids = torch.randint(0, 1000, (1, PROMPT_LEN)).to(DEVICE)
    try:
        with torch.no_grad():
            m.generate(input_ids=ids, max_new_tokens=4,
                       do_sample=False, pad_token_id=0)  # 预热
            t0 = time.time()
            m.generate(input_ids=ids, max_new_tokens=MAXNEW,
                       do_sample=False, pad_token_id=0)
            dt = time.time() - t0
        print("RESULT gen %s %s %s %.2fs total %.3fs/tok prompt=%d new=%d"
              % (TAG, DTYPE, name, dt, dt / MAXNEW, PROMPT_LEN, MAXNEW))
    except Exception as exc:
        print("RESULT gen %s %s FAILED %r" % (TAG, DTYPE, name, exc))


main()
