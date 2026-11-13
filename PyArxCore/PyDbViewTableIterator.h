#pragma once
#include "stdafx.h"
#include "PyDbAbstractViewTableIterator.h"

class PyDbViewTable;
class AcDbViewTableIterator;

// SDK: AcDbViewTableIterator : AcDbAbstractViewTableIterator（dbsymtb.h）。
class PyDbViewTableIterator : public PyDbAbstractViewTableIterator {
public:
    PyDbViewTableIterator(PyDbViewTable& owner, bool atBeginning = true, bool skipDeleted = true);
    explicit PyDbViewTableIterator(AcDbViewTableIterator* p);
    ~PyDbViewTableIterator() override = default;

    AcDbViewTableIterator* impObj();
    const AcDbViewTableIterator* impObj() const;
};

void bind_PyDbViewTableIterator(pym& m);
