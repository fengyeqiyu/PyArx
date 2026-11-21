# -*- coding: utf-8 -*-
d=r"G:\PyArx\examples\cad2d\dwg1018"
py=[l.strip() for l in open(d+r"\py_trace.txt")]
c=[l.strip() for l in open(d+r"\c_trace.txt")]
print("py",len(py),"c",len(c))
for j in range(len(py)-4, len(c)):
    print(f"{j} PY {py[j] if j<len(py) else '-'}")
    print(f"{j} C  {c[j]}")
