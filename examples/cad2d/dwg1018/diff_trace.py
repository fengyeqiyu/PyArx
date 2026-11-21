# -*- coding: utf-8 -*-
import re
d=r"G:\PyArx\examples\cad2d\dwg1018"
py=[l.strip() for l in open(d+r"\py_trace.txt")]
c=[l.strip() for l in open(d+r"\c_trace.txt")]
def norm(l):
    m=re.search(r"dec=(\d+) cb=(\d+) off=(\d+) lit=(\d+)",l)
    return tuple(m.groups()) if m else None
for i in range(min(len(py),len(c))):
    a=norm(py[i]); b=norm(c[i])
    if a!=b:
        print("FIRST DIFF @",i)
        print("PY:",py[i])
        print("C :",c[i])
        for j in range(max(0,i-3),i+3):
            print(f"  {j} PY {py[j]}")
            print(f"  {j} C  {c[j]}")
        break
else:
    print("aligned up to", min(len(py),len(c)))
