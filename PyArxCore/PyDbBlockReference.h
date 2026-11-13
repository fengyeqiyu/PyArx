#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include "PyDbAttribute.h"

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
};

void bind_PyDbBlockReference(pym& m);
