# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
want={"1","2","3","5","12","1b","20","22","23"}
found={}
def scan(ents,tag):
    for e in ents:
        h=e.dxf.handle.lower()
        if h in want: found[h]=(e.dxftype(),tag)
scan(doc.modelspace(),"MS")
for b in doc.blocks: scan(b,b.name)
# also tables
for t in doc.entitydb:
    pass
for w in want: print(w,found.get(w,"NOT FOUND"))
