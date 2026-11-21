# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
br=[]
for h,e in doc.entitydb.items():
    if e.dxftype()=="BLOCK_RECORD": br.append(int(h,16))
print("BLOCK_RECORD count",len(br),"min",hex(min(br)),"max",hex(max(br)))
print("sample",[hex(x) for x in sorted(br)[:8]])
