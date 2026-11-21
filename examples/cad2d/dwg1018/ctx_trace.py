# -*- coding: utf-8 -*-
f = r"G:\PyArx\examples\cad2d\dwg1018\trace_eed2.log"
lines = open(f, encoding="utf-8", errors="ignore").read().splitlines()
target = "v=200404"
idx = next(i for i, l in enumerate(lines) if target in l)
print("line", idx + 1)
for l in lines[max(0, idx - 10): idx + 16]:
    print(l)
