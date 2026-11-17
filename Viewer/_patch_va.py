# -*- coding: utf-8 -*-
"""把 va 光学居中补丁打进已导出的 scene.html。"""
import io
import sys

OLD = """      // va: 0=\u57fa\u7ebf 1=\u5e95 2=\u4e2d 3=\u9876 \u2014\u2014 \u771f\u6b63\u7684\u4e2d\u5bf9\u9f50\u8981\u8ba9\u5b57\u5f62\u4e2d\u5fc3\u843d\u5728\u951a\u70b9
      const vb = t[8] === 2 ? 'middle' : (t[8] === 3 ? 'top' : (t[8] === 1 ? 'bottom' : 'alphabetic'));"""
NEW = """      // va: 0=\u57fa\u7ebf 1=\u5e95 2=\u4e2d 3=\u9876 \u2014\u2014 \u771f\u6b63\u7684\u4e2d\u5bf9\u9f50\u8981\u8ba9\u5b57\u5f62\u4e2d\u5fc3\u843d\u5728\u951a\u70b9\u3002
      // ha>=3\uff08\u6b63\u4e2d/\u5e03\u6ee1/\u5bf9\u9f50\uff09\u4e14\u6ca1\u7ed9 va \u7684\u77ed\u6807\u6ce8\uff08\u8f74\u53f7 1\u202628\u3001\u56de\u8def\u53f7\uff09\u4e00\u5f8b\u6309\u5782\u76f4\u5c45\u4e2d\uff0c
      // \u8fd9\u6837\u5b83\u843d\u5728\u8f74\u53f7\u5708\u91cc\u5c31\u662f\u51e0\u4f55\u5c45\u4e2d\uff0c\u548c CAD \u663e\u793a\u4e00\u81f4\u3002
      const _short = String(t[6]).length <= 3;
      const vb = (t[8] === 2 || (t[4] >= 3 && !t[8] && _short))
        ? 'middle' : (t[8] === 3 ? 'top' : (t[8] === 1 ? 'bottom' : 'alphabetic'));"""

path = sys.argv[1]
s = io.open(path, encoding="utf-8", errors="surrogateescape").read()
c = s.count(OLD)
print("hits", c)
if c:
    s = s.replace(OLD, NEW)
    io.open(path, "w", encoding="utf-8", errors="surrogateescape").write(s)
print("done")
