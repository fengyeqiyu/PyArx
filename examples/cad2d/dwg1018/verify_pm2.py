# -*- coding: utf-8 -*-
import sys
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
from container import open_dwg
dwg = r"I:\xwechat_files\wxid_jbg0jgp7km6m21_d2a3\msg\file\2026-10\中试车间扩建-工艺配电及系统2026.10.12_t3.dwg"
c = open_dwg(dwg)
pm = c.read_system_page(0x4163003B)
pm_ref = open(r"G:\PyArx\examples\cad2d\dwg1018\pm_ref.bin","rb").read()
sm_ref = open(r"G:\PyArx\examples\cad2d\dwg1018\sm_ref.bin","rb").read()
print("PM", len(pm), "ref", len(pm_ref), "match", pm[:len(pm_ref)] == pm_ref)
sm = c.read_system_page(0x4163003B)
print("SM", len(sm), "ref", len(sm_ref), "match", sm[:len(sm_ref)] == sm_ref)
