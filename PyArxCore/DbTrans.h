#pragma once
#include "stdafx.h"
#include <AcApDocWindow.h>

class DbTrans {
private:
    Acad::ErrorStatus m_es = Acad::eOk;
    bool m_committed = false;                 // 防止 Commit 重复调用
    bool m_locked = false;                    // 本实例是否实际加上了文档锁（命令上下文不加锁）

    bool upopen(AcDbObject* pobj) {
        if (pobj->isWriteEnabled()) return true;
        auto es = pobj->upgradeOpen();
        if (es != Acad::eOk) {
            acutPrintf(L"\nupgradeOpen failed: %s\n", acadErrorStatusText(es));
            return false;
        }
        return pobj->isWriteEnabled();
    }

public:
    AcApDocument* pDoc = nullptr;
    AcDbDatabase* pDb = nullptr;
    AcDbTransactionManager* pTm = nullptr;
    AcTransaction* pTrans = nullptr;

    // 禁止拷贝 / 移动
    DbTrans(const DbTrans&) = delete;
    DbTrans& operator=(const DbTrans&) = delete;
    DbTrans(DbTrans&&) = delete;
    DbTrans& operator=(DbTrans&&) = delete;

    bool IsOk() const { return m_es == Acad::eOk; }

    DbTrans() {
        pDoc = curDoc();
        // 与 PyDbTrans::enter() 对齐：命令上下文 AutoCAD 已持写锁，重复 lockDocument 会重入死锁；
        // 仅当当前调用者未持锁（Python 顶层）时才加锁。eLockViolation 视为已被命令持锁，不加锁。
        if (pDoc->myLockMode() == AcAp::kNone) {
            m_es = acDocManagerPtr()->lockDocument(pDoc, AcAp::kWrite);
            if (m_es == Acad::eOk) m_locked = true;
            else if (m_es == Acad::eLockViolation) {
                // 命令上下文 AutoCAD 已持写锁属于预期，不是错误；恢复 m_es=eOk，
                // 否则 Commit() 的 IsOk() 分支会误走 abortTransaction，回滚本事务内 add 的实体。
                m_es = Acad::eOk;
            }
            // 其它锁失败：保持 m_es 为错误值，Commit 走 abort（合理）。
        }
        pDb = pDoc->database();

        pTm = pDb->transactionManager();
        pTrans = pTm->startTransaction();
        if (!pTrans) {
            // 事务启动失败：回滚已加的文档锁，置错误状态，避免析构空指针。
            if (m_locked) { acDocManagerPtr()->unlockDocument(pDoc); m_locked = false; }
            m_es = Acad::eNullObjectPointer;
        }
    }

    ~DbTrans() {
        Commit();
    }

    void Commit() noexcept {
        if (m_committed) return;
        m_committed = true;

        if (pTrans) {
            if (IsOk()) {
                pTm->endTransaction();
            }
            else {
                pTm->abortTransaction();
            }
        }
        // 仅释放本实例实际加上的文档锁（命令上下文由 AutoCAD 持锁，此处不解锁）。
        if (m_locked) {
            acDocManagerPtr()->unlockDocument(pDoc);
            m_locked = false;
        }
    }

    // ---------- 单个对象 ----------
    template <class T = AcDbEntity>
    T* getObject(AcDbObjectId objid,
        AcDb::OpenMode mode = AcDb::OpenMode::kForWrite,
        bool openErased = false)
    {
        T* pObj = nullptr;
        m_es = pTrans->getObject<T>(pObj, objid, mode, openErased);
        return IsOk() ? pObj : nullptr;
    }

    // ---------- 批量（AcArray）----------
    template<typename T = AcDbEntity>
    std::vector<T*> getObjects(const AcArray<AcDbObjectId>& ids,
        AcDb::OpenMode mode = AcDb::OpenMode::kForWrite,
        bool openErased = false)
    {
        std::vector<T*> ents;
        ents.reserve(ids.length());
        for (auto objid : ids) {
            T* pObj = nullptr;
            m_es = pTrans->getObject<T>(pObj, objid, mode, openErased);
            if (IsOk()) ents.push_back(pObj);
        }
        return ents;
    }

    // ---------- 批量（std::vector）----------
    template<typename T = AcDbEntity>
    std::vector<T*> getObjects(const std::vector<AcDbObjectId>& ids,
        AcDb::OpenMode mode = AcDb::OpenMode::kForWrite,
        bool openErased = false)
    {
        std::vector<T*> ents;
        ents.reserve(ids.size());
        for (auto objid : ids) {
            T* pObj = nullptr;
            m_es = pTrans->getObject<T>(pObj, objid, mode, openErased);
            if (IsOk()) ents.push_back(pObj);
        }
        return ents;
    }

    // ---------- 字典 / 表 ----------
    template<typename TableType>
    AcDbObjectId getDictId(TableType* table, const AcString& name) {
        AcDbObjectId tableId = AcDbObjectId::kNull;
        auto es = table->getAt(name, tableId);
        if (es != Acad::eOk)
            acutPrintf(L"\n不存在 %s 的记录\n", name);
        return tableId;
    }

    AcDbObjectId getObjectId(const AcDbHandle& handle) {
        AcDbObjectId objid;
        auto es = pDb->getAcDbObjectId(objid, false, handle);
        return es == Acad::eOk ? objid : AcDbObjectId::kNull;
    }

    AcDbBlockTableRecord* currentSpace() {
        return getObject<AcDbBlockTableRecord>(pDb->currentSpaceId(), AcDb::kForWrite);
    }

    AcDbBlockTableRecord* paperSpace() {
        return getObject<AcDbBlockTableRecord>(pDb->paperSpaceVportId(), AcDb::kForWrite);
    }

    AcDbBlockTable* getBlockTable() { return getObject<AcDbBlockTable>(pDb->blockTableId(), AcDb::kForWrite); }
    AcDbRegAppTable* getRegAppTable() { return getObject<AcDbRegAppTable>(pDb->regAppTableId(), AcDb::kForWrite); }
    AcDbDimStyleTable* getDimStyleTable() { return getObject<AcDbDimStyleTable>(pDb->dimStyleTableId(), AcDb::kForWrite); }
    AcDbLayerTable* getLayerTable() { return getObject<AcDbLayerTable>(pDb->layerTableId(), AcDb::kForWrite); }
    AcDbLinetypeTable* getLineTypeTable() { return getObject<AcDbLinetypeTable>(pDb->linetypeTableId(), AcDb::kForWrite); }
    AcDbTextStyleTable* getTextStyleTable() { return getObject<AcDbTextStyleTable>(pDb->textStyleTableId(), AcDb::kForWrite); }
    AcDbUCSTable* getUcsTable() { return getObject<AcDbUCSTable>(pDb->UCSTableId(), AcDb::kForWrite); }
    AcDbViewTable* getViewTable() { return getObject<AcDbViewTable>(pDb->viewTableId(), AcDb::kForWrite); }
    AcDbViewportTable* getViewportTable() { return getObject<AcDbViewportTable>(pDb->viewportTableId(), AcDb::kForWrite); }

    AcDbDictionary* getMLeaderStyleDictionary() { return getObject<AcDbDictionary>(pDb->mleaderStyleDictionaryId(), AcDb::kForWrite); }
    AcDbDictionary* getMlineStyleDictionary() { return getObject<AcDbDictionary>(pDb->mLStyleDictionaryId(), AcDb::kForWrite); }
    AcDbDictionary* getMaterialDictionary() { return getObject<AcDbDictionary>(pDb->materialDictionaryId(), AcDb::kForWrite); }
    AcDbDictionary* getPlotSettingsDictionary() { return getObject<AcDbDictionary>(pDb->plotSettingsDictionaryId(), AcDb::kForWrite); }
    AcDbDictionary* getPlotStyleNameDictionary() { return getObject<AcDbDictionary>(pDb->plotStyleNameDictionaryId(), AcDb::kForWrite); }
    AcDbDictionary* getLayoutDictionary() { return getObject<AcDbDictionary>(pDb->layoutDictionaryId(), AcDb::kForWrite); }

    // ---------- 添加实体 ----------
    AcDbObjectId addEntity(AcDbBlockTableRecord* btr, AcDbEntity* ent) {
        AcDbObjectId objId;
        if (!btr || !ent) return objId;

        Acad::ErrorStatus es = btr->appendAcDbEntity(objId, ent);
        if (es != Acad::eOk) {
            m_es = es;
            return objId;
        }

        es = pTm->addNewlyCreatedDBRObject(ent, true);
        if (es != Acad::eOk) {
            m_es = es;
            return objId;
        }
        return objId;
    }

    Acad::ErrorStatus addNewlyCreatedDBRObject(AcDbEntity* ent) {
        return pTm->addNewlyCreatedDBRObject(ent, true);
    }

    AcArray<AcDbObjectId> addEntities(AcDbBlockTableRecord* btr, const AcArray<AcDbEntity*>& ents) {
        AcArray<AcDbObjectId> objIds;
        for (AcDbEntity* ent : ents) {
            auto objId = addEntity(btr, ent);
            if (objId == AcDbObjectId::kNull) continue;
            objIds.append(objId);
        }
        return objIds;
    }

    // ---------- 块表记录 ----------
    AcDbBlockTableRecord* getOrCreateBlockTableRecord(AcDbBlockTable* pBlockTable, const AcString& name) {
        if (pBlockTable->has(name)) {
            AcDbObjectId objId;
            m_es = pBlockTable->getAt(name, objId);
            if (IsOk()) return getObject<AcDbBlockTableRecord>(objId);
        }

        AcDbBlockTableRecord* pBlockRecord = new AcDbBlockTableRecord();
        pBlockRecord->setName(name);
        m_es = pBlockTable->add(pBlockRecord);
        if (IsOk()) return pBlockRecord;

        delete pBlockRecord;
        return nullptr;
    }

    // ---------- 深拷贝 ----------
    AcDbObjectIdArray deepClone(const AcDbObjectIdArray& ids) {
        AcDbIdMapping idMap;
        idMap.setDestDb(pDb);
        AcDbObjectIdArray newIds;
        pDb->deepCloneObjects(ids, pDb->currentSpaceId(), idMap, false);
        for (auto oldId : ids) {
            AcDbIdPair pair(oldId, AcDbObjectId::kNull, Adesk::kFalse);
            if (!idMap.compute(pair)) continue;
            newIds.append(pair.value());
        }
        return newIds;
    }

    AcDbObjectId deepClone(const AcDbObjectId& objid) {
        AcDbObjectIdArray ids;
        ids.append(objid);
        auto result = deepClone(ids);
        return result.isEmpty() ? AcDbObjectId::kNull : result[0];
    }

    // ---------- 删除 / 刷新 / 焦点 ----------
    void erase(AcDbObject* pObj) {
        if (!pObj) return;
        if (pObj->isErased() || pObj->isNewObject()) return;
        if (upopen(pObj)) {
            pObj->erase();
        }
    }

    void flushScreen() {
        pTm->queueForGraphicsFlush();
    }
};

// 设置焦点到当前文档窗口
inline void setFocusDoc() {
    auto windowHwnd = acDocWindowManagerPtr()->activeDocumentWindow()->windowHandle();
    SetFocus(windowHwnd);
}