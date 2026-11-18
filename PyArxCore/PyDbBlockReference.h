#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include "PyDbAttribute.h"

class PyDbDynBlockReferenceProperty;

class PyDbBlockReference : public PyDbEntity {
	PyWrapper_H(DbBlockReference)
public:
	PyDbBlockReference(AcDbBlockReference* p) : PyDbEntity(p) {}
	PyDbBlockReference() : PyDbBlockReference(new AcDbBlockReference()) {}
	AcGePoint3d position() const;
	bool setPosition(const AcGePoint3d&);
	double rotation() const;
	bool setRotation(double);
	uint64_t blockTableRecordId() const;
	bool setBlockTableRecord(uint64_t blockRecordId);
	AcGeScale3d scaleFactors() const;
	bool setScaleFactors(const AcGeScale3d&);
	AcGeScale3d nonAnnotationScaleFactors() const;
	AcGeMatrix3d blockTransform() const;
	AcGeMatrix3d nonAnnotationBlockTransform() const;
	AcGeVector3d normal() const;
	bool setNormal(const AcGeVector3d&);
	bool treatAsAcDbBlockRefForExplode() const;
	bool explodeToOwnerSpace() const;
	AcString toString() const;
	// 新增补绑（CHM 核对）
	std::tuple<AcGePoint3d, AcGePoint3d> geomExtentsBestFit(const AcGeMatrix3d& parentXform = AcGeMatrix3d::kIdentity) const;
	bool setBlockTransform(const AcGeMatrix3d& xform);
	std::tuple<AcGePlane, int> getPlane() const;
	bool isPlanar() const;
	PyDbAttribute openAttribute(uint64_t attId, AcDb::OpenMode mode, bool openErasedOne = false) const;
	int attributeCount() const;

	// 实例属性（ATTRIB）数据：tag/txt/pos/alignPoint/h/rot/ha/va/inv/ly/ci
	py::list attribData() const;

	// ---- 属性块：ATTRIB 子实体遍历（原 attributeIterator 折叠进本体）----
	// 返回的是已按 mode 打开的 PyDbAttribute 包装（所有权随包装，close/GC 释放）
	py::list attributes(int mode = 1) const;
	PyDbAttribute attributeAt(int index, int mode = 1) const;
	PyDbAttribute attributeByTag(const AcString& tag, int mode = 1) const;
	bool hasAttributeTag(const AcString& tag) const;

	// ---- 动态块：动态属性（DynBlockReferenceProperty）遍历 ----
	bool isDynamicBlock() const;
	py::list dynamicProperties() const;
	py::list dynamicPropertyNames() const;
	PyDbDynBlockReferenceProperty dynamicProperty(const AcString& name) const;
};

void bind_PyDbBlockReference(pym& m);

// 任意 AcDbObject 的实例属性导出（内部 AcDbBlockReference::cast）。
// 供 PyDbObject 绑定调用——块定义内部的 INSERT 在 Python 侧只拿到基类，
// 无法 cast 到 PyDbBlockReference，只能从这里统一取属性。
py::list PyAttribDataFromObject(AcDbObject* pObj);
