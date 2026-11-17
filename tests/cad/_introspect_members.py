# -*- coding: utf-8 -*-
"""introspect_members.py —— 打印关键类的完整成员表 + set*/append*/create* 方法签名。"""
CLASSES = ['PyDbHatch', 'PyDbTable', 'PyDbRotatedDimension', 'PyDbAlignedDimension',
           'PyDbRadialDimension', 'PyDbDiametricDimension', 'PyDbDimStyleTableRecord',
           'PyDbBlockReference', 'PyDbMText', 'PyDbBlockTableRecord',
           'PyDbLayerTableRecord', 'PyDbTrans', 'PyDbDatabase',
           'PyDbLayerTable', 'PyDbBlockTable']
MODULE_LEVEL = ['DynBlockReference', 'DynBlockTableRecord', 'DynBlockReferenceProperty']

out = []
for n in CLASSES:
    if not hasattr(P.Db, n):
        out.append('-- %s : MISSING --' % n)
        continue
    cls = getattr(P.Db, n)
    names = [x for x in dir(cls) if not x.startswith('_')]
    out.append('-- %s (%d) --' % (n, len(names)))
    out.append('  ALL: ' + ', '.join(names))
    for m in names:
        if not (m.startswith('set') or m.startswith('append') or m.startswith('add')
                or m.startswith('create') or m.startswith('insert') or 'Loop' in m
                or 'Pattern' in m or 'Text' in m or 'Row' in m or 'Column' in m):
            continue
        try:
            doc = getattr(cls, m).__doc__ or ''
        except Exception:
            continue
        sig = ' | '.join(x.strip() for x in doc.split('\n') if x.strip())[:300]
        out.append('    %s: %s' % (m, sig))

for n in MODULE_LEVEL:
    if not hasattr(P.Db, n):
        out.append('-- %s : MISSING --' % n)
        continue
    cls = getattr(P.Db, n)
    out.append('-- %s --' % n)
    out.append('  ALL: ' + ', '.join(x for x in dir(cls) if not x.startswith('_')))

result = '\n'.join(out)
print(result[:15000])
