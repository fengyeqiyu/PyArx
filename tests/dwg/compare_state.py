# -*- coding: utf-8 -*-
"""Compare decoder output against DXF-derived truth for one sample."""
from __future__ import annotations

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, REPO)

import PyArxDwg  # noqa: E402

SAMPLES = {
    "楼层平面-10KV电缆桥架.dwg": "truth_楼层平面-10KV电缆桥架.json",
    "楼层平面-强电.dwg": "truth_楼层平面-强电.json",
}


def load(path: str):
    d = PyArxDwg.read(path)
    return d, PyArxDwg.model.drawing_to_dict(d)


def main() -> int:
    for dwg, truth_name in SAMPLES.items():
        path = os.path.join(REPO, "tests", dwg)
        truth_path = os.path.join(HERE, truth_name)
        if not (os.path.exists(path) and os.path.exists(truth_path)):
            print(f"skip {dwg}")
            continue
        d, out = load(path)
        truth = json.load(open(truth_path, encoding="utf-8"))
        print(f"\n===== {dwg}")
        print(f" layers ours={[(x['name'], x['color'], x['ltype']) for x in out['layers']]}")
        print(f" layers truth={[(x['name'], x['color'], x['ltype']) for x in truth['layers']]}")
        print(f" blocks ours={[(b['name'], b['entity_count']) for b in out['blocks']]}")
        print(f" blocks truth={[(b['name'], b['entity_count']) for b in truth['blocks']]}")
        ts = truth["summary"]
        print(f" truth summary: model={ts['model_space_entities']} "
              f"block={ts['block_internal_entities']} total={ts['model_space_entities'] + ts['block_internal_entities']}")
        print(f" ours  stats: {out['stats']['entity_total']} "
              f"model={out['stats']['by_type_model_space']} ")
        print(f" ours  by_type={out['stats']['by_type']}")
        print(f" truth by_type={ts['type_counts']}")
        texts = [e for e in out["entities"] if e["type"] in ("TEXT", "MTEXT")]
        print(f" texts ours={len(texts)}")
        for e in texts[:3]:
            print("   ", repr(e["geometry"].get("text"))[:100])
    return 0


if __name__ == "__main__":
    sys.exit(main())
