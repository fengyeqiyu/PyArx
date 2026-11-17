# -*- coding: utf-8 -*-
"""introspect_ctor.py —— 打印关键类构造函数与关键方法签名。"""
TARGETS = ('PyDbLine', 'PyDbCircle', 'PyDbArc', 'PyDbText', 'PyDbMText', 'PyDbPolyline',
           'PyDbHatch', 'PyDbTable', 'PyDbAlignedDimension', 'PyDbRotatedDimension',
           'PyDbRadialDimension', 'PyDbDiameterDimension' if hasattr(P.Db, 'PyDbDiameterDimension') else 'PyDbDiametricDimension',
           'PyDbAttributeDefinition', 'PyDbAttribute', 'PyDbBlockReference', 'PyDbInsert' if hasattr(P.Db, 'PyDbInsert') else 'PyDbMInsertBlock',
           'PyDbBlockTableRecord', 'PyDbBlockTable', 'PyDbLayerTable', 'PyDbLayerTableRecord',
           'PyDbTextStyleTableRecord', 'PyDbDimStyleTableRecord', 'PyDbSolid', 'PyDbPoint',
           'PyDbEllipse', 'PyDbSpline', 'PyDbRegion', 'PyDb3dSolid', 'PyDbTableStyle',
           'PyDbTrans', 'PyDbObject', 'PyDbEntity', 'PyDbCurve', 'PyDbHatch',
           'PyDbDynBlockTableRecord' if hasattr(P.Db, 'PyDbDynBlockTableRecord') else 'PyDbBlockTableRecord')

out = []
out.append('=== Db 模块级函数 ===')
out.append(', '.join(n for n in dir(P.Db) if not n.startswith('_') and not n.startswith('PyDb')))

for n in TARGETS:
    if not hasattr(P.Db, n):
        out.append('-- %s : MISSING' % n)
        continue
    cls = getattr(P.Db, n)
    doc = getattr(cls.__init__, '__doc__', None) or ''
    first = ' | '.join(x.strip() for x in doc.split('\n') if x.strip())[:900]
    out.append('-- %s --\n   %s' % (n, first))

result = '\n'.join(out)
print(result[:14000])
