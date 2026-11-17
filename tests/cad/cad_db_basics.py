# -*- coding: utf-8 -*-
"""cad_db_basics.py —— 在 AutoCAD 里用 PyArx 建立“数据库基础内容”全套样本。

经 cadmcp bridge（req.json/res.json）执行，日志逐行落盘 G:\\PyArx\\log\\cad_db_basics.log，
崩溃时也能看出死在哪一步。覆盖：图层、图元、文字、颜色、标注、填充、表格、块、
属性块、动态块接口。
"""
import os
import traceback

LOG = r"G:\PyArx\log\cad_db_basics.log"
_f = open(LOG, 'w', encoding='utf-8')
fails = []


def log(msg):
    _f.write(str(msg) + '\n')
    _f.flush()
    os.fsync(_f.fileno()) if False else None


def check(name, fn):
    log('>>> %s ...' % name)
    try:
        v = fn()
        log('OK   %-34s %r' % (name, v))
        return v
    except Exception as e:
        fails.append(name)
        log('FAIL %-34s %s: %s' % (name, type(e).__name__, str(e)[:220]))
        log('     ' + traceback.format_exc().splitlines()[-1][:220])
        return None


Db = P.Db
P3 = P.Ge.AcGePoint3d
P2 = P.Ge.AcGePoint2d
V3 = P.Ge.AcGeVector3d


def v3(x, y, z):
    return V3(float(x), float(y), float(z))


db = Db.curDb()
log('database: %r' % (db.getFilename() or '<unsaved>'))

LAYERS = [
    ('图元-线', 255, 0, 0),
    ('图元-面', 0, 255, 0),
    ('标注-尺寸', 255, 255, 0),
    ('文字', 0, 255, 255),
    ('块-构件', 255, 0, 255),
    ('表格-清单', 0, 128, 255),
]


def clear_ms():
    with Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        ms = tr.openObject(bt.getAt('*Model_Space', 1).objectId(), 1)
        for eid in list(ms.entityIds()):
            tr.openObject(eid, 1).erase()
    return 'cleared'


check('清空模型空间', clear_ms)


def make_layers():
    made = []
    with Db.PyDbTrans() as tr:
        lt = db.getLayerTable()
        for name, r, g, b in LAYERS:
            if lt.has(name):
                made.append(name + '(已有)')
                continue
            rec = Db.PyDbLayerTableRecord.newRecord()
            rec.setName(name)
            rec.setColor(r, g, b)
            made.append('%s=%s' % (name, lt.add(rec)))
    return made


check('创建图层', make_layers)


def make_curves():
    with Db.PyDbTrans() as tr:
        ln = Db.PyDbLine(P3(0, 0, 0), P3(100, 0, 0))
        ln.setLayer('图元-线'); ln.setColorIndex(1)
        tr.addEntity(ln)

        ci = Db.PyDbCircle(P3(160, 0, 0), v3(0, 0, 1), 40)
        ci.setLayer('图元-线'); ci.setColorIndex(2)
        tr.addEntity(ci)

        ar = Db.PyDbArc(P3(260, 0, 0), 40, 0.0, 3.141592653589793)
        ar.setLayer('图元-线'); ar.setColorIndex(3)
        tr.addEntity(ar)

        pl = Db.PyDbPolyline()
        for i, (x, y) in enumerate([(0, -80), (60, -80), (60, -20), (120, -20), (120, -60)]):
            pl.addVertexAt(i, P2(x, y), 0.0, -1.0, -1.0)
        pl.setClosed(True)
        pl.setLayer('图元-线'); pl.setColorIndex(4)
        tr.addEntity(pl)

        pt = Db.PyDbPoint(P3(160, -50, 0))
        pt.setLayer('图元-线')
        tr.addEntity(pt)

        so = Db.PyDbSolid(P3(200, -80, 0), P3(240, -80, 0), P3(200, -50, 0))
        so.setLayer('图元-面'); so.setColorIndex(3)
        tr.addEntity(so)

        el = Db.PyDbEllipse(P3(320, -60, 0), v3(0, 0, 1), v3(70, 0, 0), 0.5)
        el.setLayer('图元-线'); el.setColorIndex(5)
        tr.addEntity(el)

        sp = Db.PyDbSpline([P3(0, -140, 0), P3(60, -180, 0), P3(120, -120, 0), P3(180, -170, 0)],
                           v3(1, 1, 0), v3(1, -1, 0), 0, 3)
        sp.setLayer('图元-线'); sp.setColorIndex(6)
        tr.addEntity(sp)
    return 'line/circle/arc/polyline/point/solid/ellipse/spline'


check('创建图元', make_curves)


def make_text():
    with Db.PyDbTrans() as tr:
        tx = Db.PyDbText(P3(0, 40, 0), 'PyArx TEXT 文字', 12.0, 0.0)
        tx.setLayer('文字'); tx.setColorIndex(4)
        tr.addEntity(tx)

        mt = Db.PyDbMText()
        mt.setContents('PyArx MTEXT\\P第二行：表格/标注/填充')
        mt.setLocation(P3(160, 100, 0))
        mt.setTextHeight(8.0)
        mt.setAttachment(1)
        mt.setLayer('文字'); mt.setColorIndex(6)
        tr.addEntity(mt)
    return 'TEXT + MTEXT'


check('创建文字', make_text)


def dim_style_id():
    with Db.PyDbTrans() as tr:
        return db.getDimStyleTable().getAt('ISO-25').objectId()


def make_dims():
    sid = dim_style_id()
    with Db.PyDbTrans() as tr:
        al = Db.PyDbAlignedDimension()
        al.setXLine1Point(P3(0, 0, 0))
        al.setXLine2Point(P3(100, 0, 0))
        al.setDimLinePoint(P3(50, 18, 0))
        al.setDimensionStyle(sid); al.setDimtxt(6.0)
        al.setLayer('标注-尺寸')
        tr.addEntity(al)

        ro = Db.PyDbRotatedDimension()
        ro.setXLine1Point(P3(160, 110, 0))
        ro.setXLine2Point(P3(160, 30, 0))
        ro.setDimLinePoint(P3(120, 70, 0))
        ro.setRotation(0.0)
        ro.setDimensionStyle(sid); ro.setDimensionText('H=80')
        ro.setLayer('标注-尺寸')
        tr.addEntity(ro)

        ra = Db.PyDbRadialDimension()
        ra.setCenter(P3(160, 0, 0))
        ra.setChordPoint(P3(200, 0, 0))
        ra.setDimensionStyle(sid)
        ra.setLayer('标注-尺寸')
        tr.addEntity(ra)

        di = Db.PyDbDiametricDimension()
        di.setChordPoint(P3(120, 0, 0))
        di.setFarChordPoint(P3(200, 0, 0))
        di.setDimensionStyle(sid)
        di.setLayer('标注-尺寸')
        tr.addEntity(di)
    return 'aligned/rotated/radial/diametric'


check('创建标注', make_dims)


def make_hatch():
    with Db.PyDbTrans() as tr:
        h = Db.PyDbHatch()
        h.setPattern(1, 'ANSI31')
        h.setPatternScale(3.0)
        loop = [P2(0, -80), P2(60, -80), P2(60, -20), P2(120, -20), P2(120, -60), P2(0, -60)]
        h.appendLoopPoints(1, loop, [0.0] * len(loop))
        h.setElevation(0.0)
        h.setNormal(v3(0, 0, 1))
        h.setLayer('图元-面')
        ev = h.evaluateHatch()
        tr.addEntity(h)
    return 'ANSI31 evaluate=%r' % ev


check('创建填充', make_hatch)


def make_table():
    with Db.PyDbTrans() as tr:
        tb = Db.PyDbTable()
        tb.setSize(4, 3)
        for i in range(4):
            tb.setRowHeight(i, 12.0)
        for j in range(3):
            tb.setColumnWidth(j, 40.0)
        data = [['序号', '名称', '数量'], ['1', '桥架', '12'],
                ['2', '电缆', '30'], ['3', '端子', '48']]
        for r in range(4):
            for c in range(3):
                tb.setTextString(r, c, data[r][c])
                tb.setTextHeight(r, c, 4.5)
        tb.setPosition(P3(240, -140, 0))
        tb.setLayer('表格-清单')
        ok = tr.addEntity(tb)
    return '4x3 add=%r' % ok


check('创建表格', make_table)


def make_blocks():
    with Db.PyDbTrans() as tr:
        bt = db.getBlockTable()

        btr = Db.PyDbBlockTableRecord()
        btr.setName('PY_BLK')
        btr.appendAcDbEntity(Db.PyDbCircle(P3(0, 0, 0), v3(0, 0, 1), 10))
        btr.appendAcDbEntity(Db.PyDbLine(P3(-12, 0, 0), P3(12, 0, 0)))
        btr.appendAcDbEntity(Db.PyDbLine(P3(0, -12, 0), P3(0, 12, 0)))
        bt.add(btr)
        log('   块 PY_BLK 已建')

        atr = Db.PyDbBlockTableRecord()
        atr.setName('PY_ATTDEF')
        atr.appendAcDbEntity(Db.PyDbCircle(P3(0, 0, 0), v3(0, 0, 1), 8))
        ad1 = Db.PyDbAttributeDefinition(P3(-8, -14, 0), 'EQ-001', 'NUM', '设备编号')
        ad1.setHeight(3.0)
        atr.appendAcDbEntity(ad1)
        ad2 = Db.PyDbAttributeDefinition(P3(-8, -22, 0), '待填写', 'NAME', '设备名称')
        ad2.setHeight(3.0)
        atr.appendAcDbEntity(ad2)
        bt.add(atr)
        log('   属性块 PY_ATTDEF 已建')

        ids = {'PY_BLK': bt.getAt('PY_BLK', 0).objectId(),
               'PY_ATTDEF': bt.getAt('PY_ATTDEF', 0).objectId()}
    return ids


blk_ids = check('创建块表记录', make_blocks)


def insert_blocks():
    with Db.PyDbTrans() as tr:
        br = Db.PyDbBlockReference()
        br.setBlockTableRecord(blk_ids['PY_BLK'])
        br.setPosition(P3(400, 60, 0))
        br.setRotation(0.5)
        br.setLayer('块-构件')
        tr.addEntity(br)

        ab = Db.PyDbBlockReference()
        ab.setBlockTableRecord(blk_ids['PY_ATTDEF'])
        ab.setPosition(P3(400, -40, 0))
        ab.setLayer('块-构件')
        tr.addEntity(ab)

        att1 = Db.PyDbAttribute(P3(392, -62, 0), 'EQ-778', 'NUM')
        att1.setHeight(3.0)
        ab.appendAttribute(att1)
        att2 = Db.PyDbAttribute(P3(392, -72, 0), '配电箱', 'NAME')
        att2.setHeight(3.0)
        ab.appendAttribute(att2)
    return '2 块参照 + 2 属性'


check('插入块参照/属性', insert_blocks)


def colors():
    with Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        ms = tr.openObject(bt.getAt('*Model_Space', 1).objectId(), 1)
        n = 0
        for eid in list(ms.entityIds()):
            e = tr.openObject(eid, 1)
            k = str(e.className())
            if 'Line' in k:
                e.setRGB(0, 128, 255)
                n += 1
            elif 'Circle' in k:
                e.setColorIndex(1)
                n += 1
    return '改色 %d 个（真彩色 line / ACI circle）' % n


check('设置颜色', colors)


def dyn_probe():
    out = {'classes': [n for n in ('DynBlockReference', 'DynBlockTableRecord',
                                   'DynBlockReferenceProperty') if hasattr(Db, n)]}
    try:
        r = Db.DynBlockReference(blk_ids['PY_BLK'])
        out['ctor_blockid'] = 'ok'
        out['isDynamic'] = r.isDynamicBlock(blk_ids['PY_BLK'])
    except Exception as e:
        out['ctor_blockid'] = '%s: %s' % (type(e).__name__, str(e)[:100])
    try:
        r2 = Db.DynBlockTableRecord(blk_ids['PY_BLK'])
        out['dbtrec'] = r2.blockTableRecordId()
    except Exception as e:
        out['dbtrec'] = '%s: %s' % (type(e).__name__, str(e)[:100])
    return out


check('动态块接口探测', dyn_probe)


def verify():
    with Db.PyDbTrans() as tr:
        bt = db.getBlockTable()
        ms = tr.openObject(bt.getAt('*Model_Space', 0).objectId(), 0)
        kinds = {}
        ids = list(ms.entityIds())
        for eid in ids:
            k = str(tr.openObject(eid, 0).className())
            kinds[k] = kinds.get(k, 0) + 1
    return {'entities': len(ids), 'kinds': kinds,
            'layers': db.getLayerTable().names(),
            'blocks': db.getBlockTable().names()}


summary = check('校验模型空间', verify)

log('=== FAILS: %d %s' % (len(fails), fails))
_f.close()
result = {'fails': fails, 'summary': summary}
