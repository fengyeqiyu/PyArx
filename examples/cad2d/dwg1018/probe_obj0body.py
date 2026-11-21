# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
data=open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
print("@4 header area:",data[4:40].hex(' '))
print("obj0 size2798, body 13..2811; tail:",data[2806:2812].hex(' '))
