# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
db=doc.entitydb
for h in ["1","2","3","5","4237A","423D5","423D9"]:
    e=db.get(h)
    print(h, "MISSING" if e is None else e.dxftype())
# 前 20 个数值句柄
keys=sorted(db.keys(), key=lambda x:int(x,16))[:20]
print("--- first 20 handles ---")
for k in keys:
    e=db.get(k); print(k,e.dxftype())
