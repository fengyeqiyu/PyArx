#pragma once
#include "stdafx.h"
#include "PyDbBlockReference.h"

class PyDbMInsertBlock : public PyDbBlockReference {
	PyWrapper_H(DbMInsertBlock)
public:
	PyDbMInsertBlock(AcDbMInsertBlock* p) : PyDbBlockReference(p) {}
	PyDbMInsertBlock() : PyDbMInsertBlock(new AcDbMInsertBlock()) {}
	Adesk::UInt16 columns() const;
	bool setColumns(Adesk::UInt16 columns);
	Adesk::UInt16 rows() const;
	bool setRows(Adesk::UInt16 rows);
	double columnSpacing() const;
	bool setColumnSpacing(double spacing);
	double rowSpacing() const;
	bool setRowSpacing(double spacing);
	AcString toString() const;
	// ---- Skeleton (not bound; ObjectId ctor needs review) ----
	// AcDbMInsertBlock(const AcGePoint3d& position, AcDbObjectId blockTableRec,
	//                  Adesk::UInt16 columns, Adesk::UInt16 rows,
	//                  double colSpacing, double rowSpacing)
};

void bind_PyDbMInsertBlock(pym& m);
