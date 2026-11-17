# -*- coding: utf-8 -*-
"""probe_caps.py —— 探测建层/建块/属性块/表格/填充所需的确切 API（只读，不改库）。"""
lines = []

def t(desc, fn):
    try:
        lines.append('OK   %-42s %r' % (desc, fn()))
    except Exception as e:
        lines.append('FAIL %-42s %s: %s' % (desc, type(e).__name__, str(e)[:120]))

t('PyDbLayerTableRecord()', lambda: P.Db.PyDbLayerTableRecord())
t('PyDbTextStyleTableRecord()', lambda: P.Db.PyDbTextStyleTableRecord())
t('PyDbDimStyleTableRecord()', lambda: P.Db.PyDbDimStyleTableRecord())
t('PyDbTableStyle()', lambda: P.Db.PyDbTableStyle())

db = P.Db.curDb()
with P.Db.PyDbTrans() as tr:
    bt = db.getBlockTable()
    t('getBlockTable', lambda: (bt.numRecords(), bt.names()[:6]))
    lt = db.getLayerTable()
    t('getLayerTable', lambda: (lt.numRecords(), lt.names()[:6]))
    btr = tr.openObject(bt.getAt('*Model_Space', 0).objectId(), 0)
    t('modelspace open', lambda: (btr.getName(), len(btr.entityIds())))
    tt = db.getTextStyleTable()
    t('textstyle names', lambda: tt.names())
    dt = db.getDimStyleTable()
    t('dimstyle names', lambda: dt.names()[:6])

for cls, methods in (('PyDbBlockReference', ('appendAttribute', 'attributeIds', 'setBlockTransform')),
                     ('PyDbHatch', ('appendLoopPoints', 'setPattern', 'evaluateHatch', 'hatchStyle')),
                     ('PyDbTable', ('setSize', 'setTableStyle', 'setTextString', 'setBlockTableRecord')),
                     ('PyDbBlockTableRecord', ('appendAcDbEntity', 'setName', 'setOrigin')),
                     ('PyDbLayerTable', ('getAt', 'add')),
                     ('PyDbTrans', ('openObject', 'addEntity')),
                     ('PyDbEntity', ('setColorIndex', 'setLayer', 'setLinetype')),
                     ('PyDbText', ('position', 'setPosition', 'setHeight')),
                     ('PyDbMText', ('setAttachment', 'attachment')),
                     ('PyDbDimension', ('setDimensionText', 'setDimtxt', 'setDimensionStyle'))):
    k = getattr(P.Db, cls)
    for m in methods:
        if not hasattr(k, m):
            lines.append('MISS %s.%s' % (cls, m))
            continue
        doc = getattr(k, m).__doc__ or ''
        s = ' | '.join(x.strip() for x in doc.split('\n') if x.strip())[:220]
        lines.append('DOC  %s.%s :: %s' % (cls, m, s))

result = '\n'.join(lines)
print(result[:9000])
