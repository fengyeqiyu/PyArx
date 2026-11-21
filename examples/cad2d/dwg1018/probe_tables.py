# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
db=doc.entitydb
# list TABLE records and their handle / name
for h,e in db.items():
    if e.dxftype()=="TABLE":
        print("TABLE",h, e.dxf.get("name",""))
