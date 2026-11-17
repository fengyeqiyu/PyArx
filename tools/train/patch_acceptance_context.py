# -*- coding: utf-8 -*-
"""把本轮 LoRA 重训结果写进 acceptance.json 的 context_samples 段。"""
import json, os

P = r"G:\PyArx\tools\train\data\acceptance.json"
with open(P, encoding="utf-8") as f:
    doc = json.load(f)

doc["dataset"] = {
    "total": 204,
    "tools": 141,
    "facts_constraints": 63,
    "ratio": 0.3088,
    "note": "事实约束 63/204=30.88% ≤35% 硬线；原 sft_cad_tools 107 行内容逐字节未改。",
}

doc["eval"] = {
    "hit": 4,
    "total": 6,
    "hits": ["cad_line", "cad_circle", "cad_text", "cad_erase"],
    "misses": [
        ["统计一下当前图里有多少实体、分别在哪层", "cad_stats {}",
         "None；raw 散文串接 cad_stats/类名/图层计数，未成形调用。"],
        ["查一下 LIVE_TEST 层上都有什么", "cad_query {\"layer\":\"LIVE_TEST\"}",
         "cad_layer {\"cls\":\"AcDbLayer\",\"title\":\"LIVE_TEST\"}；查询意图被误路由到建层工具。"],
    ],
    "log": r"G:\PyArx\tools\train\logs\eval_v8.log",
    "note": "name-match 4/6，达到 ≥4/6 门槛；cad_erase 名称命中，参数仍为 handle 而非 handles。",
}

doc["adapter_rollback"] = {
    "current_adapter": r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora",
    "source": r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v3",
    "rolled_back": False,
    "note": "eval 4/6 未退化，保留新 adapter；未生成 -v6-prev / -v8-exp 留档目录。v1/v2/v3/v5-exp 保留未删。",
    "retained": [
        r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v1",
        r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v2",
        r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v3",
        r"G:\PyArx\tools\train\out\qwen0.5b-cad-lora-v5-exp",
    ],
}

doc["context_samples"] = {
    "new_prefix_rows": 28,
    "prefix_groups": {
        "layer_exists": 4,
        "handle_missing": 4,
        "stats_from_prefix": 4,
        "plain": 16,
    },
    "new_constraint_rows": 6,
    "dataset": {
        "total": 204,
        "tools": 141,
        "facts_constraints": 63,
        "ratio": 0.3088,
    },
    "training": {
        "rounds": [
            {"log": "train7_05b_204.log", "rows": 204, "epochs": 2, "threads": 6,
             "duration_s": 1313, "note": "边界轮；脚本 mask 修复前启动的首轮无效。"},
            {"log": "train8_05b_204.log", "rows": 204, "epochs": 2, "threads": 6,
             "duration_s": 1352, "note": "修后脚本有效轮，adapter 落 out\\qwen0.5b-cad-lora。"},
        ],
        "verify_mask": {"loss_tokens": 62, "note": "含 2 个 <|im_end|>，assistant 标记已 mask；60→62 量级。"},
    },
    "eval": {"score": "4/6", "hit": 4, "total": 6, "rolled_back": False},
    "stop_evidence": {
        "probe_cases": 3,
        "tool_calls_per_case": [1, 1, 1],
        "extra_calls": 0,
        "note": "三条指令单次推理各出 1 个 <tool_call>，无额外重复调用。",
    },
    "current_adapter_version": "train8 产物（out\\qwen0.5b-cad-lora，与 v3 哈希不同）；未回滚到 v3。",
}

with open(P, "w", encoding="utf-8") as f:
    json.dump(doc, f, ensure_ascii=False, indent=2)
print("context_samples written")
