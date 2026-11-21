# -*- coding: utf-8 -*-
"""对照实验：只改措辞、不动时间线，看哪些穿帮层能救回来。
结论用途：区分「词表层（可修）」和「事实层（改不了）」。
"""
from pathlib import Path

from build_and_simulate import JD

OUT = Path(__file__).resolve().parent
raw = (OUT / "resume.txt").read_text(encoding="utf-8")

# 只动措辞，一个字都不碰日期
v2 = (raw
      .replace("consumer-electronics brands", "consumer-electronics marketplace brands")
      .replace("promotion slots", "campaign slots")
      .replace("content calendars", "campaign and content calendars"))


def coverage(text):
    low = text.lower()
    must = [k for k in JD["must"] if k in low]
    nice = [k for k in JD["nice"] if k in low]
    return must, nice


m1, n1 = coverage(raw)
m2, n2 = coverage(v2)

lines = [
    "# 对照：同一份简历，只改措辞",
    "",
    "| 指标 | 原版 | 只改措辞 |",
    "|---|---|---|",
    "| 必备词命 | %d/%d | %d/%d |" % (len(m1), len(JD["must"]), len(m2), len(JD["must"])),
    "| 加分词命 | %d/%d | %d/%d |" % (len(n1), len(JD["nice"]), len(n2), len(JD["nice"])),
]
lines.append("")
lines.append("原版缺必备词：" + ("、".join(k for k in JD["must"] if k not in m1) or "无"))
lines.append("改后缺必备词：" + ("、".join(k for k in JD["must"] if k not in m2) or "无"))
lines.append("")
lines.append("结论：关键词层（L4）靠替换 3 处措辞就能补齐；"
             "时间线重叠（L3）一个字都动不了——那是经历本身的事实冲突。")
text = "\n".join(lines)
(OUT / "variant_check.md").write_text(text, encoding="utf-8")
print(text)
