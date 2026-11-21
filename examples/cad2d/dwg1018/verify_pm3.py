# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from compress import decompress
pm = open(r"G:\PyArx\examples\cad2d\dwg1018\pm_body.bin","rb").read()
ref = open(r"G:\PyArx\examples\cad2d\dwg1018\pm_ref.bin","rb").read()
out = decompress(pm, len(ref))
print("PM match:", bytes(out) == ref, len(out), len(ref))
sm = open(r"G:\PyArx\examples\cad2d\dwg1018\sm_ref.bin","rb").read()
