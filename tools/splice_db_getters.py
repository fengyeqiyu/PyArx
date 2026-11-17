"""把 docs_notes/db_getters_plan.txt 里的 @@@CODE@@@ 代码块插入 PyDbDatabase.cpp 的 class_ 链末尾。

锚点：`.def("closeInput", ...` 的收尾 `);`（链最后一条）。
插入后保持原文件编码（UTF-8 BOM）与 CRLF。
"""
import sys

CPP = r"G:\PyArx\PyArxCore\PyDbDatabase.cpp"
PLAN = r"G:\PyArx\docs_notes\db_getters_plan.txt"
ANCHOR = '}, "关闭挂起的输入队列");'

plan = open(PLAN, encoding="utf-8").read().split("@@@CODE@@@", 1)[1].strip().splitlines()
if not plan:
    sys.exit("empty plan")

raw = open(CPP, "rb").read()
bom = raw.startswith(b"\xef\xbb\xbf")
text = raw.decode("utf-8-sig")
if ANCHOR not in text:
    sys.exit("anchor not found")

# 逐条加缩进与换行
block = ["\t\t}, \"关闭挂起的输入队列\")"]
block.append("\t\t// ---- PyRx getter 对齐补齐（自动生成，纯读无副作用） ----")
for line in plan:
    block.append("\t\t" + line.strip())
block.append("\t\t;")

text = text.replace(ANCHOR, "\r\n".join(block), 1)
out = text.encode("utf-8")
if bom:
    out = b"\xef\xbb\xbf" + out
open(CPP, "wb").write(out)
print("inserted lines=%d bom=%s" % (len(plan), bom))
