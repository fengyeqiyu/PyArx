#pragma once
#include "stdafx.h"
#include "PyDbEntity.h"

class PyDbDimStyleTableRecord;

class PyDbFcf : public PyDbEntity {
	PyWrapper_H(DbFcf)
public:
	PyDbFcf(AcDbFcf* p) : PyDbEntity(p) {}
	PyDbFcf() : PyDbFcf(new AcDbFcf()) {}
	AcString text(int lineNo = -1) const;
	bool setText(const AcString&);
	AcGePoint3d location() const;
	bool setLocation(const AcGePoint3d&);
	AcGeVector3d normal() const;
	AcGeVector3d direction() const;
	bool setOrientation(const AcGeVector3d& norm, const AcGeVector3d& dir);
	double dimgap() const;
	bool setDimgap(double);
	double dimscale() const;
	bool setDimscale(double);
	double dimtxt() const;
	bool setDimtxt(double);
	bool setDimVars();
	uint64_t dimensionStyle() const;
	uint64_t dimtxsty() const;
	AcGePoint3dArray getBoundingPline() const;
	AcGePoint3dArray getBoundingPoints() const;
	bool setDimensionStyle(uint64_t styleId);
	bool setDimstyleData(uint64_t styleId);
	bool setDimtxsty(uint64_t txstyId);
	PyDbDimStyleTableRecord* getDimstyleData() const;
	AcString toString() const;
};

void bind_PyDbFcf(pym& m);
