# -*- coding: utf-8 -*-
"""stage1.py —— 逐微步探测：图层表写入、图元写入。每步先落盘再执行，崩溃可定位。"""
import os
import traceback

LOG = r"G:\PyArx\log\stage1.log"
_f = open(LOG, 'w', encoding='utf-8')


def log(m):
    _f.write(str(m) + '\n')
    _f.flush()


Db = P.Db
db = Db.curDb()
log('db=%r layerTableId=%r blockTableId=%r' % (db.getFilename(), db.layerTableId, db.blockTableId))

log('A: db.getLayerTable() 直取')
lt_direct = None
try:
    lt_direct = db.getLayerTable()
    log('A ok: numRecords=%r names=%r' % (lt_direct.numRecords(), lt_direct.names()))
except Exception:
    log('A fail: ' + traceback.format_exc().splitlines()[-1])

log('B: 事务内 openObject(layerTableId,1)')
lt_tx = None
tr_holder = None
try:
    tr = Db.PyDbTrans()
    tr_holder = tr
    lt_tx = tr.openObject(db.layerTableId, 1)
    log('B ok: type=%r' % type(lt_tx).__name__)
    log('B names=%r' % (lt_tx.names(),))
except Exception:
    log('B fail: ' + traceback.format_exc().splitlines()[-1])

log('C: 建一条图层记录（不入库）')
rec = None
try:
    rec = Db.PyDbLayerTableRecord.newRecord()
    log('C ok: %r setName=%r' % (type(rec).__name__, rec.setName('S1-LAYER')))
    log('C setColor=%r' % (rec.setColor(255, 0, 0),))
except Exception:
    log('C fail: ' + traceback.format_exc().splitlines()[-1])

log('D: lt_tx.add(rec)')
try:
    log('D add=%r' % (lt_tx.add(rec),))
    log('D names=%r' % (lt_tx.names(),))
except Exception:
    log('D fail: ' + traceback.format_exc().splitlines()[-1])

log('E: 提交事务')
try:
    tr.commit()
    log('E ok')
except Exception:
    log('E fail: ' + traceback.format_exc().splitlines()[-1])

log('F: 建线')
try:
    with Db.PyDbTrans() as tr2:
        ln = Db.PyDbLine(P.Ge.AcGePoint3d(0, 0, 0), P.Ge.AcGePoint3d(100, 0, 0))
        log('F1 line ok')
        log('F2 addEntity=%r' % (tr2.addEntity(ln),))
    log('F ok')
except Exception:
    log('F fail: ' + traceback.format_exc().splitlines()[-1])

log('G: 读回实体数')
try:
    with Db.PyDbTrans() as tr3:
        bt = tr3.openObject(db.blockTableId, 1)
        ms = tr3.openObject(bt.getAt('*Model_Space', 0).objectId(), 0)
        log('G entities=%d' % len(list(ms.entityIds())))
except Exception:
    log('G fail: ' + traceback.format_exc().splitlines()[-1])

log('END')
_f.close()
result = 'stage1 done'
