# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
db=doc.entitydb
for h in ["1","2","3","5","4237a","423d5","423d9"]:
    e=db.get(h)
    if e is None:
        print(h,"MISSING"); continue
    t=e.dxftype();
    try: owner=e.dxf.owner
    except Exception: owner="-"
    try: nm=e.dxf.name
    except Exception: nm="-"
    print(h,t,"owner",owner,"name",nm)
