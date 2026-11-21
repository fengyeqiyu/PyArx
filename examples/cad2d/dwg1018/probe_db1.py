# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
db=doc.entitydb
for h in ["1","2","3","5"]:
    e=db.get(h)
    if e:
        print(h,e.dxftype(),"name",e.dxf.get("name"))
# blockrecord name for 1,2
