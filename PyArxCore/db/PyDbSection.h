#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"
#include <DbSection.h>

class PyDbSection : public PyDbEntity {
	PyWrapper_H(DbSection)
public:
	PyDbSection() : PyDbSection(new AcDbSection()) {}
	PyDbSection(AcDbSection* p) : PyDbEntity(p) {}
	int state() const;
	bool setState(int);
	AcString getName() const;
	bool setName(const AcString&);
	AcGeVector3d viewingDirection() const;
	bool setViewingDirection(const AcGeVector3d&);
	AcGeVector3d verticalDirection() const;
	bool setVerticalDirection(const AcGeVector3d&);
	AcGeVector3d normal() const;
	int indicatorTransparency() const;
	bool setIndicatorTransparency(int);
	int numVertices() const;
	AcGePoint3d getVertex(int index) const;
	bool setVertex(int index, const AcGePoint3d&);
	bool addVertex(int index, const AcGePoint3d&);
	bool removeVertex(int index);
	AcString toString() const;
};

void bind_PyDbSection(pym& m);
