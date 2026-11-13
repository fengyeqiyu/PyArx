# -*- coding: utf-8 -*-
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import engine, pp_app

doc = pp_app.sampleDual()
nodes = doc
ENG = engine.PowerPlanEngine()
lay = ENG.layout(nodes, False)
print('nodes=%d links=%d size=%.0fx%.0f' % (len(lay['nodes']), len(lay['links']), lay['width'], lay['height']))
d = {}
for n in lay['nodes']:
    d.setdefault(n['depth'], []).append(n)
for k in sorted(d):
    xs = sorted(set(round(x['x']) for x in d[k]))
    print('depth %d: %d 节点, x=%s' % (k, len(d[k]), xs[:6]))
for n in sorted(lay['nodes'], key=lambda z:(z['depth'], z['x'])):
    print('%s d=%d x=%.0f y=%.0f w=%d' % (n['code'], n['depth'], n['x'], n['y'], n['w']))
