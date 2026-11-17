"""一次性生成 enum_baseline.txt：PyArx 手工绑定的枚举名清单（不含生成物里的名字）。

生成器改用这个固定清单判断「哪些要生成」，避免拿运行时快照当基线导致自我递归跳过。
"""
import io
import json
import re

DUMP = r"G:\PyArx\log\enum_dump.json"
GEN = r"G:\PyArx\PyArxCore\PyArxExtraEnums.cpp"
OUT = r"G:\PyArx\docs_notes\api_parity\enum_baseline.txt"

dump = json.load(open(DUMP))
gen_text = io.open(GEN, encoding="utf-8").read()
gen_names = set(re.findall(r'py::enum_<.*?>\(m, "(\w+)"', gen_text))
gen_names |= {"ColorMethod", "TransparencyMethod"}  # 别名赋值

rows = []
for mod, enums in dump.items():
    for name in enums:
        if name in gen_names:
            continue
        rows.append("%s,%s" % (mod, name))

io.open(OUT, "w", encoding="utf-8", newline="").write("\n".join(sorted(rows)) + "\n")
print("基线枚举 %d 条 -> %s" % (len(rows), OUT))
