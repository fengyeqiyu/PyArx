#pragma once
#include "stdafx.h"
#include "PyDbAbstractViewTableIterator.h"

class PyDbViewportTable;
class AcDbViewportTableIterator;

// SDK: AcDbViewportTableIterator : AcDbAbstractViewTableIterator（dbsymtb.h）。
class PyDbViewportTableIterator : public PyDbAbstractViewTableIterator {
public:
    PyDbViewportTableIterator(PyDbViewportTable& owner, bool atBeginning = true, bool skipDeleted = true);
    explicit PyDbViewportTableIterator(AcDbViewportTableIterator* p);
    ~PyDbViewportTableIterator() override = default;

    AcDbViewportTableIterator* impObj();
    const AcDbViewportTableIterator* impObj() const;
};

void bind_PyDbViewportTableIterator(pym& m);
