#pragma once
#include "stdafx.h"
#include "dbAnnotationScale.h"

// AcDbAnnotationScale pointer wrapper (obtained from the ACDB_ANNOTATIONSCALES
// context collection; engine-owned, NOT auto-deleted here). Plain pointer
// wrapper, no PyWrapper_H macro.
class PyDbAnnotationScale {
	AcDbAnnotationScale* m_ptr = nullptr;
public:
	explicit PyDbAnnotationScale(AcDbAnnotationScale* p = nullptr) : m_ptr(p) {}
	AcDbAnnotationScale* impObj() const { return m_ptr; }
	bool isNull() const { return m_ptr == nullptr; }

	AcString name() const;
	bool setName(const AcString& v);
	intptr_t uniqueIdentifier() const;
	AcString collectionName() const;

	double paperUnits() const;
	bool setPaperUnits(double v);
	double drawingUnits() const;
	bool setDrawingUnits(double v);
	double scale() const;
	bool isTemporaryScale() const;
	bool matchScaleId(intptr_t id) const;

	AcString toString() const;
};

void bind_PyDbAnnotationScale(pym& m);
