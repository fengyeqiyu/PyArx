# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
want={"42487","42488","42489","4237a","423d5","423d9"}
# iterate all entities incl tables
def scan(ents):
    for e in ents:
        h=e.dxf.handle
        if h.lower() in want:
            print(h, e.dxftype(), "layer",e.dxf.layer,"color",e.dxf.color)
scan(doc.modelspace())
for blk in doc.blocks:
    scan(blk)
# tables
for t in [doc.layers,doc.linetypes,doc.styles,doc.dimstyles,doc.appids,doc.block_records]:
    for e in t:
        h=e.dxf.handle
        if h.lower() in want:
            print("TABLE",h,e.dxftype(),getattr(e.dxf,"name",""))
