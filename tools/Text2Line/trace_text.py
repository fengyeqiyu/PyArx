"""trace_text.py —— 输入文字参数，输出与渲染一致的墨迹轮廓（em 单位，基线 y=0）。

用法：python trace_text.py <params.json>
params: {"face": "SimSun", "text": "...", "em_px": 768, "simp_px": 1.6}
输出：同目录 trace_out.json = {"loops": [[[x,y],...],...], "bbox": [minx,miny,maxx,maxy]}
"""

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import glyph_trace


def main(params_path):
    with open(params_path, encoding="utf-8") as f:
        p = json.load(f)
    loops = glyph_trace.text_contours(
        p.get("face", "SimSun"), p["text"],
        em_px=int(p.get("em_px", 768)), simp_px=float(p.get("simp_px", 1.6)))
    bbox = [min(q[0] for l in loops for q in l), min(q[1] for l in loops for q in l),
            max(q[0] for l in loops for q in l), max(q[1] for l in loops for q in l)]
    out = {"loops": loops, "bbox": bbox, "count": len(loops), "pts": sum(len(l) for l in loops)}
    with open(os.path.join(HERE, "trace_out.json"), "w", encoding="utf-8") as f:
        json.dump(out, f)
    print("[TRACE]", {"count": out["count"], "pts": out["pts"], "bbox": bbox})


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "trace_in.json"))
