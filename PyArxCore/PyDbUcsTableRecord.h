#pragma once
#include "stdafx.h"
#include "PyDbSymbolTableRecord.h"

// C++ wrapper class follows the SDK token (AcDbUCSTableRecord); the Python-visible
// name "PyDbUcsTableRecord" is set in bind_PyDbUcsTableRecord().
class PyDbUCSTableRecord : public PyDbSymbolTableRecord {
public:
	PyWrapper_H(DbUCSTableRecord)
	PyDbUCSTableRecord(AcDbUCSTableRecord* ptr) : PyDbSymbolTableRecord(ptr) {}
	PyDbUCSTableRecord() : PyDbUCSTableRecord(new AcDbUCSTableRecord()) {}

	AcGePoint3d origin() const;
	bool setOrigin(const AcGePoint3d& pt);
	AcGeVector3d xAxis() const;
	bool setXAxis(const AcGeVector3d& v);
	AcGeVector3d yAxis() const;
	bool setYAxis(const AcGeVector3d& v);
	AcGePoint3d ucsBaseOrigin(int view) const;
	bool setUcsBaseOrigin(const AcGePoint3d& pt, int view);

	// ---- Skeleton (not bound; needs review) ----
	// setUcs(origin, xAxis, yAxis) / setUcs(OrthographicView) / setUcs(ucsId)
	// belong to AcDbAbstractViewTableRecord, not this record.
};

void bind_PyDbUcsTableRecord(pym& m);
