# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
db=doc.entitydb
for h in ["1","2","3","5","12","1b","20","22","23"]:
    e=db.get(h.upper())
    print(h, e.dxftype() if e else "NONE")
