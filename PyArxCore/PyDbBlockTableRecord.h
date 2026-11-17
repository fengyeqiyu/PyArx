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

	// 块内实体数量（不含已删除实体）
	int numEntities() const;

	// 按 ObjectId(oldId) 直接打开块内实体；mode 默认 1=写，纯查询传 0
	PyDbEntity getEntityById(uint64_t id, int mode = 1) const;
	// 打开块内实体并交出所有权（供 pybind take_ownership 绑定，避免按值返回时
	// 临时包装析构把刚打开的对象 close 掉）
	PyDbEntity* openEntityById(uint64_t id, int mode = 1) const;

	// 块内实体 ObjectId 列表（不打开实体）
	py::list entityIds() const;

	// 块内属性定义（ATTDEF）数据。
	// 为什么需要 C++ 侧接口：块定义内的 ATTDEF 经 openObject 只拿到基类 PyDbObject，
	// Python 侧 cast 到 PyDbAttributeDefinition 全部失败，标签/默认值/字高都读不出来。
	// 这里在 C++ 里直接 AcDbAttributeDefinition::cast，导出 tag/txt/位置/字高/对齐/图层。
	py::list attdefData() const;

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
