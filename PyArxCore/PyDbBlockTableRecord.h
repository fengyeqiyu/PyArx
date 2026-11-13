#pragma once
#include "stdafx.h"
#include "PyDbObject.h"
#include "PyDbEntity.h"
#include "PyDbDatabase.h"
#include "PyDbSymbolTableRecord.h"

class PyDbBlockTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbBlockTableRecord)
	PyDbBlockTableRecord(AcDbBlockTableRecord* ptr) : PyDbSymbolTableRecord(ptr) {}
	AcString comments() const;
	bool setComments(const AcString& comments);
	AcString pathName() const;
	bool setPathName(const AcString& path);
	AcGePoint3d origin() const;
	bool setOrigin(const AcGePoint3d& pt);
	bool hasAttributeDefinitions() const;
	bool hasPreviewIcon() const;
	bool isAnonymous() const;
	bool isFromExternalReference() const;
	bool isFromOverlayReference() const;
	bool setIsFromOverlayReference(bool bIsOverlay);
	bool isLayout() const;
	uint64_t getLayoutId() const;
	bool setLayoutId(uint64_t layoutId);
	bool isUnloaded() const;
	bool setIsUnloaded(bool isUnloaded);
	int xrefStatus() const;
	py::list getBlockReferenceIds(bool directOnly = true, bool forceValidity = false) const;
	py::list getErasedBlockReferenceIds() const;
	bool assumeOwnershipOf(const AcDbObjectIdArray& entitiesToMove);
	int blockScaling() const;
	bool setBlockScaling(int blockScaling);
	bool explodable() const;
	bool setExplodable(bool bExplodable);
	int blockInsertUnits() const;
	bool setBlockInsertUnits(int insunits);
	int postProcessAnnotativeBTR(bool bqueryOnly = false, bool bScale = true);
	bool addAnnoScalestoBlkRefs(bool bScale = false);
	inline Acad::ErrorStatus appendAcDbEntity(PyDbEntity& pEntity) {
		return impObj()->appendAcDbEntity(pEntity.impObj());
	}

	// bool 包装版（绑定到 Python，失败返回 false）
	bool appendEntity(PyDbEntity& pEntity);

	// 追加实体并回传新实体 ObjectId（oldId，uint64）；失败返回 0
	uint64_t appendAcDbEntityId(PyDbEntity& pEntity);

	// 预览图标（PreviewIcon = AcArray<UInt8>，转 std::vector<uint8_t>）
	std::vector<uint8_t> getPreviewIcon() const;
	bool setPreviewIcon(const std::vector<uint8_t>& bytes);

	// ---- P2 依赖（对应 PyDbXxx 包装类未实现，暂不绑定）----
	// newIterator(AcDbBlockTableRecordIterator*&, bool atBeginning, bool skipDeleted)
	// openBlockBegin(AcDbBlockBegin*&, OpenMode) / openBlockEnd(AcDbBlockEnd*&, OpenMode)
	// newBlockReferenceIdIterator(AcDbBlockReferenceIdIterator*&)
	// getSortentsTable(AcDbSortentsTable*&, OpenMode, bool createIfNecessary)
	// xrefDatabase(bool incUnres) const -> AcDbDatabase*  -- raw database pointer
};

void bind_PyDbBlockTableRecord(pym& m);
