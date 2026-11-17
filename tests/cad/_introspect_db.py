# -*- coding: utf-8 -*-
"""introspect_db.py —— 通过 cadmcp bridge 摸清 PyArx.Db 的实体/图层/块表/表格/标注/填充 API。"""
import inspect

WANT = ('Hatch', 'Table', 'Dimension', 'MText', 'Text', 'Attribute', 'Block',
        'Layer', 'Linetype', 'Dynamic', 'Dyn', 'Insert', 'Circle', 'Line', 'Polyline',
        'Ellipse', 'Spline', 'Point', 'Solid', 'Region', 'Mleader', 'Leader',
        'DimStyle', 'TextStyle', 'RegApp', 'Viewport', 'Layout', 'Group')

names = [n for n in dir(P.Db) if n.startswith('PyDb')]
hits = [n for n in names if any(w.lower() in n.lower() for w in WANT)]

out = []
out.append('=== PyDb 类总数 %d ===' % len(names))
out.append('=== 相关类 ===')
for n in hits:
    out.append(n)
out.append('=== 详情 ===')
for n in hits:
    cls = getattr(P.Db, n)
    members = [m for m in dir(cls) if not m.startswith('_')]
    out.append('-- %s (%d) --' % (n, len(members)))
    out.append('   ' + ', '.join(members[:60]))

out.append('=== Ge 类 ===')
out.append(', '.join(n for n in dir(P.Ge) if n.startswith('AcGe')))

result = '\n'.join(out)
print(result[:12000])
