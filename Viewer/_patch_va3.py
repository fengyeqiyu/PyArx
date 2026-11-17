# -*- coding: utf-8 -*-
"""scene 里去掉短标注强制垂直居中，只保留 va 语义偏移。"""
import io
import sys

OLD = """      const _short = String(t[6]).length <= 3;
      const vmode = (t[8] === 2 || (t[4] >= 3 && !t[8] && _short)) ? 2
        : (t[8] === 3 ? 3 : (t[8] === 1 ? 1 : 0));"""
NEW = """      const vmode = t[8] === 2 ? 2 : (t[8] === 3 ? 3 : (t[8] === 1 ? 1 : 0));"""

path = sys.argv[1]
s = io.open(path, encoding="utf-8", errors="surrogateescape").read()
c = s.count(OLD)
print("hits", c)
if c:
    s = s.replace(OLD, NEW)
    io.open(path, "w", encoding="utf-8", errors="surrogateescape").write(s)
print("done")
