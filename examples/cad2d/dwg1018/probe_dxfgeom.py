# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
want={"42487","42488","42489"}
def scan(ents):
    for e in ents:
        if e.dxf.handle.lower() in want:
            s=e.dxf.start; t=e.dxf.end
            print(e.dxf.handle,e.dxftype(),"layer",e.dxf.layer,"color",e.dxf.color,
                  f"start=({s.x:.6f},{s.y:.6f},{s.z:.6f}) end=({t.x:.6f},{t.y:.6f},{t.z:.6f})")
scan(doc.modelspace())
for blk in doc.blocks: scan(blk)
