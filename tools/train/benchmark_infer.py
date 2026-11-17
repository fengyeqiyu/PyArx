# -*- coding: utf-8 -*-
"""推理吞吐基准：同环境 CPU 与 DirectML 对跑，只测 forward。

    python benchmark_infer.py --cpu
    python benchmark_infer.py --dml
环境变量 BENCH_MODEL / BENCH_BS / BENCH_SEQ / BENCH_REPS 可覆盖。
"""

import os
import sys
import time

MODEL = os.environ.get("BENCH_MODEL", r"D:\models\qwen2.5-0.5b")
BS = int(os.environ.get("BENCH_BS", 1))
SEQ = int(os.environ.get("BENCH_SEQ", 64))
REPS = int(os.environ.get("BENCH_REPS", 3))
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

    name = os.path.basename(MODEL)
    torch_dtype = torch.float16 if DTYPE == "float16" else torch.float32
    t0 = time.time()
    m = AutoModelForCausalLM.from_pretrained(MODEL, torch_dtype=torch_dtype)
    m = m.to(DEVICE)
    m.eval()
    load_s = time.time() - t0

    ids = torch.randint(0, 1000, (BS, SEQ)).to(DEVICE)
    try:
        with torch.no_grad():
            m(input_ids=ids)  # 预热
            t0 = time.time()
            for _ in range(REPS):
                m(input_ids=ids)
            per = (time.time() - t0) / REPS
        print("RESULT infer %s %s per=%.2fs load=%.1fs bs=%d seq=%d"
              % (TAG, name, per, load_s, BS, SEQ))
    except Exception as exc:
        print("RESULT infer %s %s FAILED %r" % (TAG, name, exc))


main()
