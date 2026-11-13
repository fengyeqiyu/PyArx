# -*- coding: utf-8 -*-
import sys, time
sys.path.insert(0, r"G:\PyArx\tests\mfc_xaml")
import pp_app, engine

p = r"G:\PyArx\docs\_big_20_10_50.csv"
t0 = time.time(); text = open(p, encoding="utf-8").read(); t1 = time.time()
rows = pp_app.App.parseCsvText(text); t2 = time.time()
nodes = pp_app.App.nodesFromRows(rows); t3 = time.time()
eng = engine.PowerPlanEngine()
plan = eng.layout(nodes); t4 = time.time()
sc = eng.scene(nodes); t5 = time.time()
print("read %.0fms parse %.0fms nodesFromRows %.0fms layout %.0fms scene %.0fms" %
      ((t1-t0)*1000, (t2-t1)*1000, (t3-t2)*1000, (t4-t3)*1000, (t5-t4)*1000))
print("rows=%d nodes=%d boxes=%d links=%d canvas=%sx%s" % (len(rows), len(nodes), len(plan["boxes"]), len(plan["links"]), plan.get("width"), plan.get("height")))
print("warnings=%d" % len(plan.get("warnings", [])))