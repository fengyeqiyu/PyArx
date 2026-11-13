# -*- coding: utf-8 -*-
import cProfile, pstats, io, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import engine, pp_app

src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '_tree3_engine.py'), encoding='utf-8').read()
ns = {'__file__': os.path.abspath(__file__), '__name__': '__main__'}
exec(compile(src.split("ENG = engine.PowerPlanEngine()")[0], 'gen', 'exec'), ns)
nodes = ns['nodes']
ENG = engine.PowerPlanEngine()
pr = cProfile.Profile()
pr.enable()
ENG.layout(nodes, False)
pr.disable()
s = io.StringIO()
pstats.Stats(pr, stream=s).sort_stats('cumtime').print_stats(12)
print(s.getvalue())
