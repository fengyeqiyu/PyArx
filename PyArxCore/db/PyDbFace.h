#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbFace : public PyDbEntity {
	PyWrapper_H(DbFace)
public:
	PyDbFace(AcDbFace* p) : PyDbEntity(p) {}
	PyDbFace() : PyDbFace(new AcDbFace()) {}
	PyDbFace(const AcGePoint3d& pt0, const AcGePoint3d& pt1, const AcGePoint3d& pt2,
		bool e0vis = true, bool e1vis = true, bool e2vis = true, bool e3vis = true)
		: PyDbEntity(new AcDbFace(pt0, pt1, pt2,
			e0vis ? Adesk::kTrue : Adesk::kFalse, e1vis ? Adesk::kTrue : Adesk::kFalse,
			e2vis ? Adesk::kTrue : Adesk::kFalse, e3vis ? Adesk::kTrue : Adesk::kFalse)) {}
	PyDbFace(const AcGePoint3d& pt0, const AcGePoint3d& pt1, const AcGePoint3d& pt2, const AcGePoint3d& pt3,
		bool e0vis = true, bool e1vis = true, bool e2vis = true, bool e3vis = true)
		: PyDbEntity(new AcDbFace(pt0, pt1, pt2, pt3,
			e0vis ? Adesk::kTrue : Adesk::kFalse, e1vis ? Adesk::kTrue : Adesk::kFalse,
			e2vis ? Adesk::kTrue : Adesk::kFalse, e3vis ? Adesk::kTrue : Adesk::kFalse)) {}
	AcGePoint3d getVertexAt(Adesk::UInt16 index) const;
	bool setVertexAt(Adesk::UInt16 index, const AcGePoint3d&);
	bool isEdgeVisibleAt(Adesk::UInt16 index) const;
	bool makeEdgeVisibleAt(Adesk::UInt16 index);
	bool makeEdgeInvisibleAt(Adesk::UInt16 index);
	AcString toString() const;
};

void bind_PyDbFace(pym& m);
