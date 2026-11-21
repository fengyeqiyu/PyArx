# -*- coding: utf-8 -*-
import ezdxf
doc=ezdxf.readfile(r"G:\PyArx\examples\cad2d\dwg1018\ground_truth.dxf")
def hx(e): return int(e.dxf.handle,16)
msp=list(doc.modelspace())
blk=[]
for b in doc.blocks:
    blk+=list(b)
print("modelspace entities",len(msp),"block entities",len(blk))
ms=[hx(e) for e in msp]; bk=[hx(e) for e in blk]
print("MS handle min/max",hex(min(ms)),hex(max(ms)))
print("BLK handle min/max",hex(min(bk)),hex(max(bk)))
print("MS sample",[hex(x) for x in ms[:5]])
# counts by block
from collections import Counter
c=Counter(b.name for b in doc.blocks)
print("num blocks",len(doc.blocks))
