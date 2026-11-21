# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
msp=doc.modelspace()
lines=[e for e in msp if e.dxftype()=="LINE"]
print("LINE count",len(lines))
for e in lines[:6]:
    h=e.dxf.handle
    layer=e.dxf.layer
    color=e.dxf.color
    ltype=e.dxf.linetype
    s=e.dxf.start; t=e.dxf.end
    print(f"h={h} layer={layer} color={color} ltype={ltype} start=({s.x:.3f},{s.y:.3f},{s.z:.3f}) end=({t.x:.3f},{t.y:.3f},{t.z:.3f})")
# layer table
print("--- layers ---")
for l in doc.layers:
    print(l.dxf.handle, l.dxf.name, l.dxf.color, l.dxf.linetype)
